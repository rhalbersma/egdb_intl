# Overview 

The DLL gives access to the endgame databases from languaages other than C++. 

See files in the VBA_test directory for access from VBA.

## Functions
For the functions below that return an int value, unless otherwise stated a negative value indicates an error. See error values in `egdb_intl_dll.h`.
Some of the DLL function below are similar to the C++ driver functions described in the `master` repository branch. You may find more info on those function is the master branch docs.
### extern "C" int __stdcall egdb_open(char *options, int cache_mb, const char *directory, const char *filename);
Opens a database for access, and returns a handle that can be passed to other access   functions.
Options, cache_mb, and directory are identical to the C++ version of this function (see Master branch).
Filename is the name of a logging file. The driver will write status and error messages to this file. Give an empty string ("") for no file.
### extern "C" int __stdcall egdb_close(int handle);
Closes the driver pointed to by `handle` and releases all associated resources. 
### extern "C" int __stdcall egdb_identify(const char *directory, int *egdb_type, int *max_pieces);
Checks if an endgame database exists in `directory`. If so, the identified endgame database type is written into `egdb_type` and the maximum number of pieces for any of the databases identified is written into `max_pieces`. Otherwise, these out-parameters are unchanged on return.

Returns zero if a database is found, non-zero otherwise.

### extern "C" int __stdcall egdb_lookup_fen(int handle, char *fen, int cl);
Returns the value of a position in the database. `fen` is the position in FEN notation. Example: "B:WK3,K9,49:B2,7,13,37".
### extern "C" int __stdcall egdb_lookup_fen_with_search(int handle, char *fen);
Does a search and returns the WLD value of a position in a WLD database. The WLD database may exclude captures and other positions to make the compression work better, but these values can be found using a search.
`fen` is the position in FEN notation. Example: "B:WK3,K9,49:B2,7,13,37".
### extern "C" int __stdcall egdb_lookup_with_search(int handle, egdb_interface::BOARD *board, int color);
Does a search and returns the WLD value of a position in a WLD database. The WLD database does not have values for all positions meeting the max_pieces constraints, but these values can be found using a search.
`board` is the position in bitboard format.
### extern "C" int __stdcall egdb_lookup_distance(int handle_wld, int handle_dist, const char *fen, int *return_size, int distances[MAXMOVES], char moves[MAXMOVES][20]);
Returns distance values in plies and their corresponding best-moves from a distance to win (DTW) or distance to conversion (MTC) database. Values in the `distances` and  `moves` arrays are ordered as best moves first. For example, if the position is a win, then move[0] is the move leading to the shortest distance to win, and distances[0] is that distance in plies. The function requires handles to both a WLD db and a distance db (DTW or MTC).
`return_size` returns the number of elements returned in `distances` and `moves`.
### extern "C" int __stdcall get_movelist(egdb_interface::BOARD *board, int color, egdb_interface::BOARD *ml);
Returns in the array `ml` a list of successor positions reachable by legal moves from the start position given by `board `and`color`. The function return value is the number of moves in `ml`.
### extern "C" int16_t __stdcall is_capture(egdb_interface::BOARD *board, int color);
The function returns non-zero if the position has capture moves.
### extern "C" int64_t __stdcall getdatabasesize_slice(int nbm, int nbk, int nwm, int nwk);
Returns the number of positions in a database subset defined by number of black men, black kings, white men, and white kings.
### extern "C" void __stdcall indextoposition(int64_t index, egdb_interface::BOARD *p, int nbm, int nbk, int nwm, int nwk);
Converts an index into a database subset defined by number of black men, black kings, white men, and white kings into a position.
### extern "C" int64_t __stdcall positiontoindex(egdb_interface::BOARD *pos, int nbm, int nbk, int nwm, int nwk);
Converts a position into an index into a database subset with the given number of pieces of each type.
### extern "C" int16_t __stdcall is_sharp_win(int handle, egdb_interface::BOARD *board, int color, egdb_interface::BOARD *sharp_move_pos);
Returns non-zero if the position defined by `board` and `color` has exactly one move to win.
### extern "C" int __stdcall move_string(egdb_interface::BOARD *last_board, egdb_interface::BOARD *new_board, int color, char *move);
Given a draughts move that changes the position from `last_board` to `new_board`, this function returns the PDN character string representation of the move in `move`.
### extern "C" int __stdcall positiontofen(egdb_interface::BOARD *board, int color, char *fen);
Converts a draughts position given in bitboard format to FEN character string format.
### extern "C" int __stdcall fentoposition(char *fen, egdb_interface::BOARD *pos, int *color);
Converts a draughts position given in FEN character string format to bitboard format.
# License

    Original work Copyright (C) 2007-2020 Ed Gilbert

Distributed under the Boost Software License, Version 1.0.
(See accompanying file [LICENSE_1_0.txt](LICENSE_1_0.txt) or copy at
[http://www.boost.org/LICENSE_1_0.txt](http://www.boost.org/LICENSE_1_0.txt))
