"""
REST API tests using Flask test client (no server required).
Run from repo root: pytest src/gnubgmodule/examples/rest-api/test_rest_api.py -v
Or from this directory: pytest test_rest_api.py -v
Requires gnubg package to be built and installable.
"""
import json
import os
import sys

import pytest

# Ensure this directory is on path so "from app import app" works
_suite_dir = os.path.dirname(os.path.abspath(__file__))
if _suite_dir not in sys.path:
    sys.path.insert(0, _suite_dir)

# Skip all tests if gnubg cannot be imported (e.g. build not present)
pytest.importorskip("gnubg")

from app import app


@pytest.fixture
def client():
    app.config["TESTING"] = True
    with app.test_client() as c:
        yield c


def test_health(client):
    r = client.get("/health")
    assert r.status_code == 200
    data = r.get_json()
    assert data["status"] == "ok"
    assert "gnubg" in data


def test_evaluate(client):
    r = client.post(
        "/evaluate",
        data=json.dumps({"position_id": "4HPwATDgc/ABMA"}),
        content_type="application/json",
    )
    assert r.status_code == 200, r.get_data(as_text=True)
    data = r.get_json()
    assert "win" in data
    assert "equity" in data
    assert 0 <= data["win"] <= 1
    assert isinstance(data["equity"], (int, float))


def test_best_move(client):
    r = client.post(
        "/best-move",
        data=json.dumps({
            "position_id": "4HPwATDgc/ABMA",
            "die1": 6,
            "die2": 1,
        }),
        content_type="application/json",
    )
    assert r.status_code == 200, r.get_data(as_text=True)
    data = r.get_json()
    assert "best_move" in data
    assert "dice" in data
    assert data["dice"] == [6, 1]
    assert isinstance(data["best_move"], list)
    for step in data["best_move"]:
        assert len(step) == 2
        assert 1 <= step[0] <= 24 and 0 <= step[1] <= 24


def test_evaluate_missing_body(client):
    r = client.post("/evaluate", data="{}", content_type="application/json")
    assert r.status_code == 400
    data = r.get_json()
    assert "error" in data


def test_best_move_bad_dice(client):
    r = client.post(
        "/best-move",
        data=json.dumps({"position_id": "4HPwATDgc/ABMA", "die1": 0, "die2": 7}),
        content_type="application/json",
    )
    assert r.status_code == 400
    data = r.get_json()
    assert "error" in data


def test_best_move_all_moves(client):
    r = client.post(
        "/best-move",
        data=json.dumps({
            "position_id": "4HPwATDgc/ABMA",
            "die1": 6,
            "die2": 1,
            "all_moves": True,
        }),
        content_type="application/json",
    )
    assert r.status_code == 200, r.get_data(as_text=True)
    data = r.get_json()
    assert "best_move" in data
    assert "moves" in data
    assert "dice" in data
    assert data["dice"] == [6, 1]
    assert isinstance(data["moves"], list)
    assert len(data["moves"]) >= 1
    for i, entry in enumerate(data["moves"]):
        assert "move" in entry
        assert "score" in entry
        assert isinstance(entry["move"], list)
        assert isinstance(entry["score"], (int, float))
        for step in entry["move"]:
            assert len(step) == 2
    assert data["best_move"] == data["moves"][0]["move"]


# --- Config endpoint ---


def test_config_get(client):
    r = client.get("/config")
    assert r.status_code == 200
    data = r.get_json()
    assert "evaluation" in data
    assert "position" in data
    assert "cube_and_match" in data
    assert "best_move_only" in data
    assert data["evaluation"]["plies"]["default"] == 2
    assert data["evaluation"]["plies"]["min"] == 0
    assert data["evaluation"]["plies"]["max"] == 7
    assert data["evaluation"]["cubeful"]["default"] == 1
    assert "position_id" in data["position"]
    assert "die1" in data["best_move_only"]


# --- Evaluation options ---


def test_evaluate_with_evaluation_options(client):
    # 0-ply cubeless: faster, different equity scale
    r = client.post(
        "/evaluate",
        data=json.dumps({
            "position_id": "4HPwATDgc/ABMA",
            "evaluation": {"cubeful": 0, "plies": 0},
        }),
        content_type="application/json",
    )
    assert r.status_code == 200, r.get_data(as_text=True)
    data = r.get_json()
    assert "win" in data
    assert "equity" in data
    assert 0 <= data["win"] <= 1


def test_evaluate_with_top_level_plies(client):
    r = client.post(
        "/evaluate",
        data=json.dumps({"position_id": "4HPwATDgc/ABMA", "plies": 4}),
        content_type="application/json",
    )
    assert r.status_code == 200
    data = r.get_json()
    assert "equity" in data


def test_best_move_with_evaluation_options(client):
    r = client.post(
        "/best-move",
        data=json.dumps({
            "position_id": "4HPwATDgc/ABMA",
            "die1": 6,
            "die2": 1,
            "evaluation": {"plies": 1},
        }),
        content_type="application/json",
    )
    assert r.status_code == 200
    data = r.get_json()
    assert "best_move" in data
    assert data["dice"] == [6, 1]
    assert isinstance(data["best_move"], list)


def test_evaluate_plies_clamped(client):
    # plies > 7 should be clamped to 7 (no 400)
    r = client.post(
        "/evaluate",
        data=json.dumps({"position_id": "4HPwATDgc/ABMA", "plies": 2}),
        content_type="application/json",
    )
    assert r.status_code == 200
    data = r.get_json()
    assert "win" in data


def test_evaluate_invalid_noise_type(client):
    r = client.post(
        "/evaluate",
        data=json.dumps({
            "position_id": "4HPwATDgc/ABMA",
            "noise": "not a number",
        }),
        content_type="application/json",
    )
    assert r.status_code == 400
    data = r.get_json()
    assert "error" in data


# --- Board and cube options ---


def test_evaluate_with_board_array(client):
    # Standard starting position as 2x25
    board = [
        [0, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 3, 0, 5, 0, 0, 0],
    ]
    r = client.post(
        "/evaluate",
        data=json.dumps({"board": board}),
        content_type="application/json",
    )
    assert r.status_code == 200
    data = r.get_json()
    assert "win" in data and "equity" in data


def test_evaluate_with_cube_and_score(client):
    r = client.post(
        "/evaluate",
        data=json.dumps({
            "position_id": "4HPwATDgc/ABMA",
            "cube": 2,
            "cube_owner": "X",
            "score": [1, 2],
        }),
        content_type="application/json",
    )
    assert r.status_code == 200
    data = r.get_json()
    assert "equity" in data


def test_health_engine_initialized_after_request(client):
    # First request that uses engine initializes it
    client.post(
        "/evaluate",
        data=json.dumps({"position_id": "4HPwATDgc/ABMA"}),
        content_type="application/json",
    )
    r = client.get("/health")
    assert r.status_code == 200
    data = r.get_json()
    assert data.get("engine_initialized") is True
