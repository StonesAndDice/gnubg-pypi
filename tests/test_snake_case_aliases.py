"""
Test snake_case aliases for gnubg C extension functions.
Verifies that both camelCase and snake_case names refer to the same underlying callables
and that they produce identical results when called.
"""
import unittest
import gnubg


class TestSnakeCaseAliasesExist(unittest.TestCase):
    """Test that all snake_case aliases exist and are callable."""

    def test_all_snake_case_aliases_exist(self):
        """Test that all expected snake_case aliases are available as module attributes."""
        aliases = [
            'position_id', 'position_from_id', 'position_key', 'position_from_key',
            'cube_info', 'pos_info', 'eval_context', 'rollout_context',
            'luck_rating', 'error_rating', 'parse_move', 'move_tuple_to_string',
            'find_best_move', 'find_best_moves', 'match_id', 'gnubg_id',
            'dice_rolls', 'match_checksum', 'position_bearoff', 'position_from_bearoff',
            'eq_to_mwc', 'eq_to_mwc_stderr', 'mwc_to_eq', 'mwc_to_eq_stderr',
            'get_eval_hint_filter', 'set_eval_hint_filter', 'next_turn', 'set_gnubg_id',
            'update_ui'
        ]
        for alias in aliases:
            self.assertTrue(hasattr(gnubg, alias), f"gnubg.{alias} does not exist")
            self.assertTrue(callable(getattr(gnubg, alias)), f"gnubg.{alias} is not callable")


class TestSnakeCaseAliasIdentity(unittest.TestCase):
    """Test that snake_case aliases refer to the same underlying callables as original names."""

    def test_position_aliases_identity(self):
        """Test position-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.position_id, gnubg.positionid)
        self.assertIs(gnubg.position_from_id, gnubg.positionfromid)
        self.assertIs(gnubg.position_key, gnubg.positionkey)
        self.assertIs(gnubg.position_from_key, gnubg.positionfromkey)

    def test_context_aliases_identity(self):
        """Test context-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.cube_info, gnubg.cubeinfo)
        self.assertIs(gnubg.pos_info, gnubg.posinfo)
        self.assertIs(gnubg.eval_context, gnubg.evalcontext)
        self.assertIs(gnubg.rollout_context, gnubg.rolloutcontext)

    def test_rating_aliases_identity(self):
        """Test rating-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.luck_rating, gnubg.luckrating)
        self.assertIs(gnubg.error_rating, gnubg.errorrating)

    def test_move_aliases_identity(self):
        """Test move-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.parse_move, gnubg.parsemove)
        self.assertIs(gnubg.move_tuple_to_string, gnubg.movetupletostring)
        self.assertIs(gnubg.find_best_move, gnubg.findbestmove)
        self.assertIs(gnubg.find_best_moves, gnubg.findbestmoves)

    def test_match_aliases_identity(self):
        """Test match-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.match_id, gnubg.matchid)
        self.assertIs(gnubg.gnubg_id, gnubg.gnubgid)
        self.assertIs(gnubg.match_checksum, gnubg.matchchecksum)

    def test_dice_aliases_identity(self):
        """Test dice-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.dice_rolls, gnubg.dicerolls)

    def test_bearoff_aliases_identity(self):
        """Test bearoff-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.position_bearoff, gnubg.positionbearoff)
        self.assertIs(gnubg.position_from_bearoff, gnubg.positionfrombearoff)

    def test_equity_conversion_aliases_identity(self):
        """Test equity/MWC conversion snake_case aliases reference the same functions."""
        self.assertIs(gnubg.eq_to_mwc, gnubg.eq2mwc)
        self.assertIs(gnubg.eq_to_mwc_stderr, gnubg.eq2mwc_stderr)
        self.assertIs(gnubg.mwc_to_eq, gnubg.mwc2eq)
        self.assertIs(gnubg.mwc_to_eq_stderr, gnubg.mwc2eq_stderr)

    def test_filter_aliases_identity(self):
        """Test filter-related snake_case aliases reference the same functions."""
        self.assertIs(gnubg.get_eval_hint_filter, gnubg.getevalhintfilter)
        self.assertIs(gnubg.set_eval_hint_filter, gnubg.setevalhintfilter)

    def test_misc_aliases_identity(self):
        """Test miscellaneous snake_case aliases reference the same functions."""
        self.assertIs(gnubg.next_turn, gnubg.nextturn)
        self.assertIs(gnubg.set_gnubg_id, gnubg.setgnubgid)
        self.assertIs(gnubg.update_ui, gnubg.updateui)


class TestSnakeCaseFunctionalEquivalence(unittest.TestCase):
    """Test that snake_case and camelCase names produce identical results."""

    def setUp(self):
        """Set up standard test board."""
        self.start_board = (
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (0, 2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        )
        self.start_position_id = "4HPwATDgc/ABMA"

    def test_position_id_equivalent(self):
        """Test position_id and positionid produce identical results."""
        result_snake = gnubg.position_id(self.start_board)
        result_camel = gnubg.positionid(self.start_board)
        self.assertEqual(result_snake, result_camel)
        self.assertEqual(result_snake, self.start_position_id)

    def test_position_from_id_equivalent(self):
        """Test position_from_id and positionfromid produce identical results."""
        board_snake = gnubg.position_from_id(self.start_position_id)
        board_camel = gnubg.positionfromid(self.start_position_id)
        self.assertEqual(board_snake, board_camel)
        self.assertEqual(board_snake, self.start_board)

    def test_position_key_equivalent(self):
        """Test position_key and positionkey produce identical results."""
        key_snake = gnubg.position_key(self.start_board)
        key_camel = gnubg.positionkey(self.start_board)
        self.assertEqual(key_snake, key_camel)
        self.assertIsInstance(key_snake, tuple)
        self.assertEqual(len(key_snake), 10)

    def test_position_from_key_equivalent(self):
        """Test position_from_key and positionfromkey produce identical results."""
        key = gnubg.position_key(self.start_board)
        board_snake = gnubg.position_from_key(key)
        board_camel = gnubg.positionfromkey(key)
        self.assertEqual(board_snake, board_camel)

    def test_position_roundtrip_with_snake_case(self):
        """Test board -> position_id -> position_from_id -> position_id roundtrip."""
        pos_id1 = gnubg.position_id(self.start_board)
        board = gnubg.position_from_id(pos_id1)
        pos_id2 = gnubg.position_id(board)
        self.assertEqual(pos_id1, pos_id2)

    def test_position_key_roundtrip_with_snake_case(self):
        """Test board -> position_key -> position_from_key -> position_key roundtrip."""
        key1 = gnubg.position_key(self.start_board)
        board = gnubg.position_from_key(key1)
        key2 = gnubg.position_key(board)
        self.assertEqual(key1, key2)

    def test_cube_info_equivalent(self):
        """Test cube_info and cubeinfo produce identical results."""
        info_snake = gnubg.cube_info(2, -1, 0, 5, (0, 0), 0)
        info_camel = gnubg.cubeinfo(2, -1, 0, 5, (0, 0), 0)
        self.assertEqual(info_snake, info_camel)
        self.assertIsInstance(info_snake, dict)
        self.assertIn('cube', info_snake)

    def test_pos_info_equivalent(self):
        """Test pos_info and posinfo produce identical results."""
        info_snake = gnubg.pos_info(0, 0, 0, 1, (1, 2))
        info_camel = gnubg.posinfo(0, 0, 0, 1, (1, 2))
        self.assertEqual(info_snake, info_camel)
        self.assertIsInstance(info_snake, dict)
        self.assertIn('dice', info_snake)

    def test_eval_context_equivalent(self):
        """Test eval_context and evalcontext produce identical results."""
        ctx_snake = gnubg.eval_context(0, 2, 1, 0, 0.0)
        ctx_camel = gnubg.evalcontext(0, 2, 1, 0, 0.0)
        self.assertEqual(ctx_snake, ctx_camel)
        self.assertIsInstance(ctx_snake, dict)

    def test_rollout_context_equivalent(self):
        """Test rollout_context and rolloutcontext produce identical results."""
        ctx_snake = gnubg.rollout_context()
        ctx_camel = gnubg.rolloutcontext()
        self.assertEqual(ctx_snake, ctx_camel)
        self.assertIsInstance(ctx_snake, dict)

    def test_eq_to_mwc_equivalent(self):
        """Test eq_to_mwc and eq2mwc produce identical results."""
        cubeinfo = gnubg.cube_info(2, -1, 0, 5, (0, 0), 0)
        result_snake = gnubg.eq_to_mwc(0.5, cubeinfo)
        result_camel = gnubg.eq2mwc(0.5, cubeinfo)
        self.assertEqual(result_snake, result_camel)
        self.assertIsInstance(result_snake, float)

    def test_mwc_to_eq_equivalent(self):
        """Test mwc_to_eq and mwc2eq produce identical results."""
        cubeinfo = gnubg.cube_info(2, -1, 0, 5, (0, 0), 0)
        result_snake = gnubg.mwc_to_eq(0.5, cubeinfo)
        result_camel = gnubg.mwc2eq(0.5, cubeinfo)
        self.assertEqual(result_snake, result_camel)
        self.assertIsInstance(result_snake, float)

    def test_parse_move_equivalent(self):
        """Test parse_move and parsemove produce identical results."""
        move_str = "8/5 6/5"
        result_snake = gnubg.parse_move(move_str)
        result_camel = gnubg.parsemove(move_str)
        self.assertEqual(result_snake, result_camel)
        self.assertIsInstance(result_snake, tuple)

    def test_match_id_equivalent(self):
        """Test match_id and matchid produce identical results."""
        cubeinfo = gnubg.cube_info(2, -1, 0, 5, (0, 0), 0)
        posinfo = gnubg.pos_info(0, 0, 0, 1, (1, 2))
        result_snake = gnubg.match_id(cubeinfo, posinfo)
        result_camel = gnubg.matchid(cubeinfo, posinfo)
        self.assertEqual(result_snake, result_camel)
        self.assertIsInstance(result_snake, str)

    def test_gnubg_id_equivalent(self):
        """Test gnubg_id and gnubgid produce identical results."""
        cubeinfo = gnubg.cube_info(2, -1, 0, 5, (0, 0), 0)
        posinfo = gnubg.pos_info(0, 0, 0, 1, (1, 2))
        result_snake = gnubg.gnubg_id(self.start_board, cubeinfo, posinfo)
        result_camel = gnubg.gnubgid(self.start_board, cubeinfo, posinfo)
        self.assertEqual(result_snake, result_camel)
        self.assertIn(':', result_snake)

    def test_luck_rating_equivalent(self):
        """Test luck_rating and luckrating produce identical results."""
        result_snake = gnubg.luck_rating(0.05)
        result_camel = gnubg.luckrating(0.05)
        self.assertEqual(result_snake, result_camel)
        self.assertIsInstance(result_snake, int)

    def test_error_rating_equivalent(self):
        """Test error_rating and errorrating produce identical results."""
        result_snake = gnubg.error_rating(0.05)
        result_camel = gnubg.errorrating(0.05)
        self.assertEqual(result_snake, result_camel)
        self.assertIsInstance(result_snake, int)


class TestSnakeCaseErrorHandling(unittest.TestCase):
    """Test that snake_case aliases handle errors the same way as original names."""

    def test_invalid_position_id_error_handling(self):
        """Test that position_from_id raises same error as positionfromid for invalid input."""
        with self.assertRaises(ValueError):
            gnubg.position_from_id("INVALID_ID")
        with self.assertRaises(ValueError):
            gnubg.positionfromid("INVALID_ID")

    def test_invalid_board_format_error_handling(self):
        """Test that position_id raises same error as positionid for invalid board."""
        with self.assertRaises(TypeError):
            gnubg.position_id("not a board")
        with self.assertRaises(TypeError):
            gnubg.positionid("not a board")

    def test_invalid_key_format_error_handling(self):
        """Test that position_from_key raises same error as positionfromkey for invalid key."""
        with self.assertRaises(TypeError):
            gnubg.position_from_key("not a key")
        with self.assertRaises(TypeError):
            gnubg.positionfromkey("not a key")


if __name__ == '__main__':
    unittest.main()
