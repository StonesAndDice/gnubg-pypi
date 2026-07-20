try:
    # Import from the uniquely named _version module
    from ._version import version, git_revision, short_version, full_version
    __version__ = version
except ImportError:
    version = "unknown"
    __version__ = "0.0.0"
    git_revision = "unknown"
    short_version = "0.0.0"

# Import all functions from the C++ extension module
# REMOVED try/except to reveal build/link errors
from ._gnubg import *
from . import _gnubg as _gnubg_ext

# Snake_case aliases for the C-extension's compact function names.
# Original camelCase names remain available for backward compatibility.
position_id = _gnubg_ext.positionid
position_from_id = _gnubg_ext.positionfromid
position_key = _gnubg_ext.positionkey
position_from_key = _gnubg_ext.positionfromkey
cube_info = _gnubg_ext.cubeinfo
pos_info = _gnubg_ext.posinfo
eval_context = _gnubg_ext.evalcontext
rollout_context = _gnubg_ext.rolloutcontext
luck_rating = _gnubg_ext.luckrating
error_rating = _gnubg_ext.errorrating
parse_move = _gnubg_ext.parsemove
move_tuple_to_string = _gnubg_ext.movetupletostring
find_best_move = _gnubg_ext.findbestmove
find_best_moves = _gnubg_ext.findbestmoves
match_id = _gnubg_ext.matchid
gnubg_id = _gnubg_ext.gnubgid
dice_rolls = _gnubg_ext.dicerolls
match_checksum = _gnubg_ext.matchchecksum
position_bearoff = _gnubg_ext.positionbearoff
position_from_bearoff = _gnubg_ext.positionfrombearoff
eq_to_mwc = _gnubg_ext.eq2mwc
eq_to_mwc_stderr = _gnubg_ext.eq2mwc_stderr
mwc_to_eq = _gnubg_ext.mwc2eq
mwc_to_eq_stderr = _gnubg_ext.mwc2eq_stderr
get_eval_hint_filter = _gnubg_ext.getevalhintfilter
set_eval_hint_filter = _gnubg_ext.setevalhintfilter
next_turn = _gnubg_ext.nextturn
set_gnubg_id = _gnubg_ext.setgnubgid
update_ui = _gnubg_ext.updateui