# REST API example – best move and cube evaluation

A minimal REST API that uses the **gnubg** Python package to return the best move for a given position and dice roll, and to evaluate a position (equity and cube-relevant probabilities).

## Prerequisites

- Python 3.10+
- **gnubg** installed (`pip install gnubg` or build from the [gnubg-pypi](https://github.com/StonesAndDice/gnubg-pypi) source)
- GNUBG neural net and data files (so that `gnubg.command("new session")` can load the evaluator)

## Install and run

From this directory:

```bash
pip install -r requirements.txt
flask --app app run --no-reload
```

Use `--no-reload` because the gnubg native extension can segfault when the Flask dev server forks for reloading. The app runs with `threaded=False` so that all requests are handled on the main thread (the engine’s thread-local state is only valid there). By default the server listens on `http://127.0.0.1:5000`. Use `--host 0.0.0.0` to listen on all interfaces.

**If you installed gnubg in editable mode** (`pip install -e .` from the repo root), ensure the package is built first so the native extension exists. From the **repository root** (e.g. `gnubg-pypi/`), run:

```bash
pip install -e .
```

Then run the Flask app from `examples/rest-api/` as above. If you see `FileNotFoundError` for `build/cp310` (or similar), the editable build is out of date—run `pip install -e .` from the repo root again.

### Troubleshooting

- **Server exits or segfaults on first `/evaluate` or `/best-move`**  
  As of a recent fix, the gnubg Python module initializes neural nets and match equity when the module is first imported (so the first request should no longer crash). If you still see a crash:
  1. Rebuild and reinstall the package from the repository root: `pip install -e .`
  2. Ensure the gnubg data files (e.g. `gnubg.wd`, `gnubg_ts0.bd`, `met/`) are on the data path (run from the repo root or set the data directory appropriately).
  3. As a workaround you can try: `GNUBG_SKIP_SESSION=1 flask --app app run --no-reload`

- **Segmentation fault after first request (e.g. after GET /health)**  
  The engine uses thread-local state that exists only on the main thread. Run the app with `threaded=False` (the default in `app.py` when you run `python app.py --no-reload`). If you use a production WSGI server, use a single worker process and ensure requests are not dispatched to extra threads.

- **503 "Engine not initialized"**  
  Engine init failed (e.g. missing data files). The error message in the JSON body may indicate the cause.

## Endpoints

### `GET /health`

Checks that the app and gnubg are available.

```bash
curl http://127.0.0.1:5000/health
```

### `GET /config`

Returns the available configuration options for request bodies (evaluation settings, position/cube fields). Use this to discover valid keys, types, and defaults.

```bash
curl http://127.0.0.1:5000/config
```

### `POST /best-move`

Returns the best move for a position and a dice roll.

**Request body (JSON):**

| Field         | Type   | Required | Description |
|---------------|--------|----------|-------------|
| `position_id` | string | one of   | GNUBG 14-character position ID (e.g. `"4HPwATDgc/ABMA"`) |
| `board`       | array  | one of   | 2×25 board: `[[x0..x24], [o0..o24]]` |
| `die1`, `die2`| int    | yes*     | Dice values 1–6. Or use `dice: [die1, die2]`. |
| `cube`, `cube_owner`, `side_to_move`, `match_to`, `score`, `crawford` | | no | Same as in `/evaluate` |
| `evaluation`  | object | no       | Evaluation settings: `cubeful`, `plies` (0–7), `deterministic`, `prune`, `noise`. Or set top-level `plies`, `cubeful`, etc. |
| `all_moves`   | bool   | no       | If `true`, response includes `moves`: all legal moves ordered by score (best first), each `{"move": [[from,to],...], "score": float}`. `best_move` is the first. |

**Example:**

```bash
curl -X POST http://127.0.0.1:5000/best-move \
  -H "Content-Type: application/json" \
  -d '{"position_id": "4HPwATDgc/ABMA", "die1": 6, "die2": 1}'
```

**Example response:**

```json
{
  "best_move": [[13, 19], [8, 9]],
  "dice": [6, 1]
}
```

Move format is a list of `[from_point, to_point]` pairs (1-based board points).

### `POST /evaluate`

Returns position evaluation: win/gammon/backgammon probabilities and cubeful equity.

**Request body (JSON):**

| Field           | Type   | Required | Description |
|-----------------|--------|----------|-------------|
| `position_id`   | string | one of  | GNUBG position ID |
| `board`         | array  | one of  | 2×25 board |
| `cube`          | int    | no      | Cube value (default 1) |
| `cube_owner`    | string | no      | `"C"` (centered), `"X"`, or `"O"` (default `"C"`) |
| `side_to_move`  | string | no      | `"O"` or `"X"` (default `"O"`) |
| `match_to`      | int    | no      | Match length; 0 = money game (default) |
| `score`         | array  | no      | `[x_score, o_score]` (default `[0, 0]`) |
| `crawford`      | int    | no      | Crawford game flag (default 0) |
| `evaluation`    | object | no      | Evaluation settings: `cubeful` (0/1), `plies` (0–7), `deterministic` (0/1), `prune` (0/1), `noise` (float). Or set `plies`, `cubeful`, etc. at top level. Default: cubeful=1, plies=2. |

**Example:**

```bash
curl -X POST http://127.0.0.1:5000/evaluate \
  -H "Content-Type: application/json" \
  -d '{"position_id": "4HPwATDgc/ABMA"}'
```

**Example response:**

```json
{
  "win": 0.52,
  "win_gammon": 0.12,
  "win_backgammon": 0.01,
  "lose_gammon": 0.10,
  "lose_backgammon": 0.01,
  "equity": 0.048
}
```

Equity is from the perspective of the side to move (cubeful when applicable).

## License

Same as the gnubg-pypi project.
