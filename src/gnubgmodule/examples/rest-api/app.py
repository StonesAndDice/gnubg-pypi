"""
REST API example for the gnubg Python package.

Exposes:
  - POST /best-move  – best move for a position and dice roll
  - POST /evaluate   – position/cube evaluation (equity and outcome probabilities)

Requires the gnubg package and neural net data. Run with:
  flask --app app run --no-reload
  (--no-reload avoids segfaults when the gnubg native extension is loaded after fork.)
"""
import json
import os

try:
    import gnubg
except ImportError as e:
    raise ImportError(
        "gnubg package is required. Install with: pip install gnubg"
    ) from e

from flask import Flask, request, jsonify

app = Flask(__name__)

# One-time engine init so neural nets are loaded (required for evaluate/findbestmove)
_engine_initialized = False
_engine_init_error = None

# Set GNUBG_SKIP_SESSION=1 to skip "new session" (e.g. if it segfaults in your build).
# Some builds load neural nets lazily on first evaluate/findbestmove.
_SKIP_SESSION = os.environ.get("GNUBG_SKIP_SESSION", "").strip() in ("1", "true", "yes")


def ensure_engine():
    """Initialize gnubg engine (load neural nets). Returns (True, None) or (False, error_msg)."""
    global _engine_initialized, _engine_init_error
    if _engine_initialized:
        return (True, None)
    if _engine_init_error is not None:
        return (False, _engine_init_error)
    if _SKIP_SESSION:
        # Assume engine is usable without "new session" (lazy load).
        _engine_initialized = True
        return (True, None)
    try:
        gnubg.command("new session")
        _engine_initialized = True
        return (True, None)
    except Exception as e:
        _engine_init_error = str(e)
        return (False, _engine_init_error)


def parse_board(body):
    """Build a 2x25 board from JSON. Accepts either position_id or board."""
    if "position_id" in body:
        pid = body["position_id"]
        if not isinstance(pid, str):
            raise ValueError("position_id must be a string")
        return gnubg.positionfromid(pid)
    if "board" in body:
        raw = body["board"]
        if not isinstance(raw, (list, tuple)) or len(raw) != 2:
            raise ValueError("board must be a list of two lists of 25 integers")
        for row in raw:
            if not isinstance(row, (list, tuple)) or len(row) != 25:
                raise ValueError("each board row must be 25 integers")
        return (tuple(map(int, raw[0])), tuple(map(int, raw[1])))
    raise ValueError("request body must include either 'position_id' or 'board'")


def parse_cube(body):
    """Build cubeinfo from optional JSON. Defaults: cube=1, centered, money game."""
    # gnubg.cubeinfo(nCube, fCubeOwner, fMove, nMatchTo, (anScore0, anScore1), fCrawford)
    # fCubeOwner: -1 centered, 0 X, 1 O
    n_cube = int(body.get("cube", 1))
    owner = body.get("cube_owner", "C")  # C, X, O
    f_move = 0 if body.get("side_to_move", "O").upper() in ("O", "0") else 1
    n_match_to = int(body.get("match_to", 0))
    score = body.get("score", (0, 0))
    if isinstance(score, (list, tuple)) and len(score) == 2:
        an_score = (int(score[0]), int(score[1]))
    else:
        an_score = (0, 0)
    f_crawford = int(body.get("crawford", 0))
    f_cube_owner = -1 if owner.upper() == "C" else (0 if owner.upper() == "X" else 1)
    return gnubg.cubeinfo(n_cube, f_cube_owner, f_move, n_match_to, an_score, f_crawford)


# Default evaluation: cubeful, 2-ply, deterministic, no prune, no noise
_DEFAULT_EVAL = {"cubeful": 1, "plies": 2, "deterministic": 1, "prune": 0, "noise": 0.0}


def parse_evalcontext(body):
    """
    Build evalcontext from request body.
    Accepts an "evaluation" object or top-level keys: cubeful, plies, deterministic, prune, noise.
    All optional; missing values use defaults.
    """
    opts = dict(_DEFAULT_EVAL)
    eval_block = body.get("evaluation")
    if isinstance(eval_block, dict):
        for k in ("cubeful", "plies", "deterministic", "prune"):
            if k in eval_block and eval_block[k] is not None:
                opts[k] = int(eval_block[k])
        if "noise" in eval_block and eval_block["noise"] is not None:
            opts["noise"] = float(eval_block["noise"])
    for k in ("cubeful", "plies", "deterministic", "prune"):
        if k in body and body[k] is not None:
            opts[k] = int(body[k])
    if "noise" in body and body["noise"] is not None:
        opts["noise"] = float(body["noise"])
    # Clamp plies to 0–7 (engine uses 4 bits)
    plies = opts["plies"]
    opts["plies"] = max(0, min(7, plies))
    return gnubg.evalcontext(
        opts["cubeful"], opts["plies"], opts["deterministic"], opts["prune"], opts["noise"]
    )


@app.route("/config", methods=["GET"])
def config():
    """
    Return available configuration options for requests (evaluation and cube/position).
    Use these keys in POST /evaluate and POST /best-move bodies.
    """
    return jsonify({
        "evaluation": {
            "cubeful": {"type": "int", "default": 1, "description": "1 = cubeful, 0 = cubeless"},
            "plies": {"type": "int", "default": 2, "min": 0, "max": 7, "description": "Evaluation depth in plies"},
            "deterministic": {"type": "int", "default": 1, "description": "1 = deterministic, 0 = with noise"},
            "prune": {"type": "int", "default": 0, "description": "Use pruning nets (1) or not (0)"},
            "noise": {"type": "float", "default": 0.0, "min": 0.0, "description": "Noise standard deviation (0 = none)"},
        },
        "position": {
            "position_id": {"type": "string", "description": "GNUBG 14-char position ID (e.g. 4HPwATDgc/ABMA)"},
            "board": {"type": "array", "description": "2×25 board [[x0..x24], [o0..o24]]"},
        },
        "cube_and_match": {
            "cube": {"type": "int", "default": 1},
            "cube_owner": {"type": "string", "default": "C", "enum": ["C", "X", "O"]},
            "side_to_move": {"type": "string", "default": "O", "enum": ["O", "X"]},
            "match_to": {"type": "int", "default": 0},
            "score": {"type": "array", "default": [0, 0], "description": "[x_score, o_score]"},
            "crawford": {"type": "int", "default": 0},
        },
        "best_move_only": {
            "die1": {"type": "int", "min": 1, "max": 6},
            "die2": {"type": "int", "min": 1, "max": 6},
            "dice": {"type": "array", "description": "Alternative to die1/die2: [die1, die2]"},
            "all_moves": {"type": "bool", "default": False, "description": "If true, response includes all legal moves ordered by score (best first), each with move and score."},
        },
    })


@app.route("/health", methods=["GET"])
def health():
    # Do not call ensure_engine() here; keep health lightweight and safe
    return jsonify({
        "status": "ok",
        "gnubg": "available",
        "engine_initialized": _engine_initialized,
    })


def _move_tuple_to_pairs(move_tuple):
    """Convert (from, to, from, to, ...) to [[from, to], ...]."""
    if not move_tuple:
        return []
    return [
        [move_tuple[i], move_tuple[i + 1]]
        for i in range(0, len(move_tuple) - 1, 2)
    ]


@app.route("/best-move", methods=["POST"])
def best_move():
    """
    Request JSON: position_id (str) or board (2x25), die1, die2 (1-6),
    optional cube/match and evaluation fields.
    If all_moves is true: response includes "moves" (all legal moves ordered by
    score, best first) each {"move": [[from,to],...], "score": float}, plus "best_move".
    Otherwise: response is best_move and dice only.
    """
    ok, err = ensure_engine()
    if not ok:
        return jsonify({"error": f"Engine not initialized: {err}"}), 503
    try:
        body = request.get_json(force=True, silent=True) or {}
        board = parse_board(body)
        cubeinfo = parse_cube(body)
        ec = parse_evalcontext(body)
        die1 = int(body.get("die1", body.get("dice", [0, 0])[0]))
        die2 = int(body.get("die2", body.get("dice", [0, 0])[1]))
        if not (1 <= die1 <= 6 and 1 <= die2 <= 6):
            return jsonify({"error": "die1 and die2 must be 1-6"}), 400
        all_moves = body.get("all_moves") is True

        if all_moves:
            moves_with_scores = gnubg.findbestmoves(board, cubeinfo, ec, (die1, die2))
            moves = [
                {"move": _move_tuple_to_pairs(entry["move"]), "score": entry["score"]}
                for entry in moves_with_scores
            ]
            best_move_list = moves[0]["move"] if moves else []
            return jsonify({
                "best_move": best_move_list,
                "moves": moves,
                "dice": [die1, die2],
            })
        move = gnubg.findbestmove(board, cubeinfo, ec, (die1, die2))
        move_list = _move_tuple_to_pairs(move)
        return jsonify({"best_move": move_list, "dice": [die1, die2]})
    except ValueError as e:
        return jsonify({"error": str(e)}), 400
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route("/evaluate", methods=["POST"])
def evaluate():
    """
    Request JSON: position_id (str) or board (2x25), and optional cube/match fields:
    cube (int), cube_owner (C|X|O), side_to_move (O|X), match_to (int), score (list of 2 ints), crawford (0|1).
    Response: win, win_gammon, win_backgammon, lose_gammon, lose_backgammon, equity (cubeful).
    """
    ok, err = ensure_engine()
    if not ok:
        return jsonify({"error": f"Engine not initialized: {err}"}), 503
    try:
        body = request.get_json(force=True, silent=True) or {}
        board = parse_board(body)
        cubeinfo = parse_cube(body)
        ec = parse_evalcontext(body)
        out = gnubg.evaluate(board, cubeinfo, ec)
        return jsonify({
            "win": out[0],
            "win_gammon": out[1],
            "win_backgammon": out[2],
            "lose_gammon": out[3],
            "lose_backgammon": out[4],
            "equity": out[5],
        })
    except ValueError as e:
        return jsonify({"error": str(e)}), 400
    except Exception as e:
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    ensure_engine()
    # use_reloader=False: forking with the gnubg native extension can cause segfaults
    # threaded=False: engine thread-local state exists only on the main thread; workers would segfault
    app.run(
        host="0.0.0.0",
        port=int(os.environ.get("PORT", 5000)),
        use_reloader=False,
        threaded=False,
    )
