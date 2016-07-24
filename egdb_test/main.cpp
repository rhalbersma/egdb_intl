#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "project.h"
#include "board.h"
#include "bool.h"
#include "bitcount.h"
#include "egdb_intl.h"
#include "egdb_search.h"
#include "move_api.h"
#include "reverse.h"
#include "bicoef.h"
#include "indexing.h"
#include "fen.h"

#define FAST_TEST_MULT 5
//#define FAST_TEST_MULT 1

#define DB_RUNLEN "c:/db_intl/wld_runlen"		/* Need 6 pieces for test. */
//#define DB_TUN_V1 "c:/db_intl/wld_v1"			/* Need 7 pieces for test. */
#define DB_TUN_V1 "e:/db_intl/wld_v1"			/* Need 7 pieces for test. */
#define DB_TUN_V2 "c:/db_intl/wld_v2"			/* Need 8 pieces for test. */
//#define DB_MTC "C:/db_intl/mtc"				/* Need 8 pieces for test. */
#define DB_MTC "e:/db_intl/slice32_mtc"			/* Need 8 pieces for test. */

#define TDIFF(start) (((double)(clock() + 1 - start)) / (double)CLOCKS_PER_SEC)

typedef struct {
	EGDB_DRIVER *handle;
	int max_pieces;
	EGDB_TYPE egdb_type;
	bool excludes_some_nonside_captures;	/* If more than 6 pieces. */
	bool excludes_some_sidetomove_colors;	/* If more than 6 pieces. */
} DB_INFO;


class SLICE {
public:
	void reset(void);
	int advance(void);
	int getnbm(void) {return(nbm);}
	int getnbk(void) {return(nbk);}
	int getnwm(void) {return(nwm);}
	int getnwk(void) {return(nwk);}
	int getnpieces(void) {return(npieces);}

private:
	const int maxpiece = 5;
	int nbm;
	int nbk;
	int nwm;
	int nwk;
	int nb;
	int nw;
	int npieces;
};


void SLICE::reset(void)
{
	npieces = 2;
	nb = 1;
	nw = 1;
	nbm = 0;
	nbk = 1;
	nwm = 0;
	nwk = 1;
}


int SLICE::advance(void)
{
	if (nwk > 0) {
		--nwk;
		++nwm;
	}
	else if (nbk > 0) {
		--nbk;
		++nbm;
		if (nb == nw) {
			nwm = nbm;
			nwk = nbk;
		}
		else {
			nwk = nw;
			nwm = 0;
		}
	}
	else if (nb < min(maxpiece, npieces - 1)) {
		++nb;
		--nw;
		nbm = 0;
		nbk = nb;
		nwm = 0;
		nwk = npieces - nb;
	}
	else {
		++npieces;
		nb = (npieces + 1) / 2;
		nw = npieces - nb;
		nbm = 0;
		nbk = nb;
		nwm = 0;
		nwk = nw;
	}
	return(npieces);
}


void print_msgs(char *msg)
{
	printf(msg);
}


void verify(DB_INFO *db1, DB_INFO *db2, SLICE *slice, int max_lookups)
{
	int64 size, index, incr;
	int value1, value2;
	bool black_ok, white_ok;
	EGDB_POSITION pos;

	printf("Verifying db%d%d%d%d\n", slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
	size = getdatabasesize_slice(slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
	if (max_lookups < 1)
		incr = 1;
	else
		incr = max(size / max_lookups, 1);

	/* If npieces >= 7, some slices do not have positions for both colors. */
	black_ok = true;
	white_ok = true;
	if (slice->getnpieces() >= 7) {
		if (db1->excludes_some_sidetomove_colors) {
			indextoposition_slice(0, &pos, slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
			value1 = db1->handle->lookup(db1->handle, &pos, BLACK, 0);
			if (value1 == EGDB_UNKNOWN)
				black_ok = false;
			value1 = db1->handle->lookup(db1->handle, &pos, WHITE, 0);
			if (value1 == EGDB_UNKNOWN)
				white_ok = false;
		}
		if (db2->excludes_some_sidetomove_colors) {
			indextoposition_slice(0, &pos, slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
			value2 = db2->handle->lookup(db2->handle, &pos, BLACK, 0);
			if (value2 == EGDB_UNKNOWN)
				black_ok = false;
			value2 = db2->handle->lookup(db2->handle, &pos, WHITE, 0);
			if (value2 == EGDB_UNKNOWN)
				white_ok = false;
		}
		if (black_ok == false && white_ok == false) {
			printf("Between db1 and db2, neither black or white can be probed. Shouldn't be possible.\n");
			exit(1);
		}
	}

	for (index = 0; index < size; index += incr) {
		indextoposition_slice(index, &pos, slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
		if (slice->getnpieces() >= 7) {
			if (db1->excludes_some_nonside_captures || db2->excludes_some_nonside_captures) {
				if (canjump((BOARD *)&pos, BLACK))
					continue;

				if (canjump((BOARD *)&pos, WHITE))
					continue;
			}
		}
		if (black_ok && !canjump((BOARD *)&pos, BLACK)) {
			value1 = db1->handle->lookup(db1->handle, &pos, BLACK, 0);
			value2 = db2->handle->lookup(db2->handle, &pos, BLACK, 0);
			if (value1 != value2) {
				printf("Verify error. index %I64d, color BLACK, v1 %d, v2 %d\n", index, value1, value2);
			}
		}

		if (white_ok && !canjump((BOARD *)&pos, WHITE)) {
			value1 = db1->handle->lookup(db1->handle, &pos, WHITE, 0);
			value2 = db2->handle->lookup(db2->handle, &pos, WHITE, 0);
			if (value1 != value2) {
				printf("Verify error. index %I64d, color WHITE, v1 %d, v2 %d\n", index, value1, value2);
			}
		}
	}
}


void self_verify(EGDB_INFO *db, SLICE *slice, int max_lookups)
{
	int64 size, index, incr;
	int value1, value2;
	bool black_ok, white_ok;
	EGDB_POSITION pos;

	printf("self verifying db%d%d%d%d\n", slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
	size = getdatabasesize_slice(slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
	if (max_lookups < 1)
		incr = 1;
	else
		incr = max(size / max_lookups, 1);

	/* If npieces >= 7, some slices do not have positions for both colors. */
	black_ok = true;
	white_ok = true;
	if (slice->getnpieces() >= 7) {
		if (db->egdb_excludes_some_nonside_caps) {
			indextoposition_slice(0, &pos, slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
			value1 = db->handle->lookup(db->handle, &pos, BLACK, 0);
			if (value1 == EGDB_UNKNOWN)
				black_ok = false;
			value1 = db->handle->lookup(db->handle, &pos, WHITE, 0);
			if (value1 == EGDB_UNKNOWN)
				white_ok = false;
		}
		if (black_ok == false && white_ok == false) {
			printf("Neither black or white can be probed. Shouldn't be possible.\n");
			exit(1);
		}
	}

	for (index = 0; index < size; index += incr) {
		indextoposition_slice(index, &pos, slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
		if (db->requires_nonside_capture_test((BOARD *)&pos)) {
			if (canjump((BOARD *)&pos, BLACK))
				continue;

			if (canjump((BOARD *)&pos, WHITE))
				continue;
		}
		if (black_ok && !canjump((BOARD *)&pos, BLACK)) {
			value1 = db->handle->lookup(db->handle, &pos, BLACK, 0);
			value2 = db->lookup_with_search((BOARD *)&pos, BLACK, 64, true);
			if (value1 != value2 && value2 != EGDB_UNKNOWN) {
				printf("Verify error. index %I64d, color BLACK, v1 %d, v2 %d\n", index, value1, value2);
			}
		}

		if (white_ok && !canjump((BOARD *)&pos, WHITE)) {
			value1 = db->handle->lookup(db->handle, &pos, WHITE, 0);
			value2 = db->lookup_with_search((BOARD *)&pos, WHITE, 64, true);
			if (value1 != value2 && value2 != EGDB_UNKNOWN) {
				printf("Verify error. index %I64d, color WHITE, v1 %d, v2 %d\n", index, value1, value2);
			}
		}
	}
}


void verify_indexing(SLICE *slice, int max_tests)
{
	int64 size, index, return_index, incr;
	EGDB_POSITION pos;

	size = getdatabasesize_slice(slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
	if (max_tests < 1)
		incr = 1;
	else
		incr = max(size / max_tests, 1);

	for (index = 0; index < size; index += incr) {
		indextoposition_slice(index, &pos, slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
		return_index = position_to_index_slice(&pos, slice->getnbm(), slice->getnbk(), slice->getnwm(), slice->getnwk());
		if (return_index != index) {
			printf("index verify error, index %I64d, return index %I64d\n", index, return_index);
			exit(1);
		}
	}
}


int identify(char *dir, DB_INFO *db)
{
	int status;

	status = egdb_identify(dir, &db->egdb_type, &db->max_pieces);
	if (status) {
		printf("db not found at %s\n", dir);
		return(1);
	}
	if (db->egdb_type == EGDB_WLD_TUN_V2) {
		db->excludes_some_nonside_captures = true;
		db->excludes_some_sidetomove_colors = true;
	}
	else {
		db->excludes_some_nonside_captures = false;
		db->excludes_some_sidetomove_colors = false;
	}
	return(0);
}


bool is_conversion_move(BOARD *from, BOARD *to, int color)
{
	uint64 from_bits, to_bits, moved_bits;

	/* It is a conversion move if its a capture. */
	from_bits = from->black | from->white;
	to_bits = to->black | to->white;
	if (bitcount64(from_bits) > bitcount64(to_bits))
		return(true);

	/* Return true if there is a man move. */
	moved_bits = from_bits & ~to_bits;
	if (moved_bits & from->king)
		return(false);
	else
		return(true);

}


void test_best_mtc_successor(EGDB_INFO *wld, EGDB_DRIVER *mtc, BOARD *pos, int color)
{
	int fromsq, tosq;
	int i, value, mtc_value, parent_value, parent_mtc_value, best_movei, best_mtc_value;
	MOVELIST movelist;
	char fenbuf[120];

	parent_value = wld->lookup_with_search(pos, color, MAXREPDEPTH, false);
	if (parent_value == EGDB_WIN)
		best_mtc_value = 1000;
	else if (parent_value == EGDB_LOSS)
		best_mtc_value = 0;
	else if (parent_value == EGDB_UNKNOWN)
		return;
	else {
		printf("bad wld value, %d, in test_best_mtc_successor\n", parent_value);
		return;
	}
	parent_mtc_value = mtc->lookup(mtc, (EGDB_POSITION *)pos, color, 0);

	build_movelist(pos, color, &movelist);
	best_movei = 0;
	for (i = 0; i < movelist.count; ++i) {
		if (is_conversion_move(pos, movelist.board + i, color)) {
			continue;
		}
		value = wld->lookup_with_search((movelist.board + i), OTHER_COLOR(color), MAXREPDEPTH, false);
		if (value == EGDB_UNKNOWN)
			return;
		if (parent_value == EGDB_WIN)
			if (value != EGDB_LOSS)
				continue;
		mtc_value = mtc->lookup(mtc, (EGDB_POSITION *)(movelist.board + i), OTHER_COLOR(color), 0);
		if (mtc_value != MTC_LESS_THAN_THRESHOLD) {
			if (parent_value == EGDB_WIN) {
				if (value == EGDB_LOSS) {
					if (mtc_value < best_mtc_value) {
						best_mtc_value = mtc_value;
						best_movei = i;
					}
				}
			}
			else {
				if (parent_value == EGDB_LOSS) {
					if (value == EGDB_WIN) {
						if (mtc_value > best_mtc_value) {
							best_mtc_value = mtc_value;
							best_movei = i;
						}
					}
				}
			}
		}
	}
	get_fromto(pos, movelist.board + best_movei, color, &fromsq, &tosq);
	if (parent_mtc_value >= 12) {
		if (best_mtc_value == parent_mtc_value || best_mtc_value == parent_mtc_value - 2) {
			test_best_mtc_successor(wld, mtc, movelist.board + best_movei, OTHER_COLOR(color));
		}
		else {
			print_fen(movelist.board + best_movei, OTHER_COLOR(color), fenbuf);
			printf("move %d-%d, parent mtc value %d, best successor mtc value %d\n",
					1 + fromsq, 1 + tosq, parent_mtc_value, best_mtc_value);
			return;
		}
	}
}


/*
 * Read a file of the highest MTC positions in each db subdivision.
 * Lookup the MTC value of that position, and then descend into one of the
 * best successor positions. Keep going until there are no successors with
 * MTC values >= 12. Make sure that each position has a successor with either
 * the same MTC value or a value that is smaller by 2. MTC values are always even
 * because the db stores the real MTC value divided by 2.
 */
void mtc_test()
{
	int color, status, max_pieces, value, max_pieces_1side, max_9pc_kings, max_8pc_kings_1side;
	FILE *fp;
	EGDB_TYPE type;
	const char *mtc_stats_filename = "10x10 HighMtc.txt";
	char linebuf[120];
	BOARD pos;
	EGDB_INFO wld;
	EGDB_DRIVER *mtc;

	printf("\nMTC test.\n");
	status = egdb_identify(DB_MTC, &type, &max_pieces);
	if (status) {
		printf("MTC db not found at %s\n", DB_MTC);
		exit(1);
	}
	if (type != EGDB_MTC_RUNLEN) {
		printf("Wrong db type, not MTC.\n");
		exit(1);
	}
	if (max_pieces != 8) {
		printf("Need 8 pieces MTC for test, only found %d.\n", max_pieces);
		exit(1);
	}

	/* Open the endgame db drivers. */
	wld.handle = egdb_open("maxpieces=8", 3000, DB_TUN_V2, print_msgs);
	if (!wld.handle) {
		printf("Cannot open tun v2 db\n");
		exit(1);
	}
	wld.handle->get_pieces(wld.handle, &max_pieces, &max_pieces_1side, &max_9pc_kings, &max_8pc_kings_1side);
	wld.dbpieces = max_pieces;
	wld.dbpieces_1side = max_pieces_1side;
	wld.egdb_excludes_some_nonside_caps = true;

	mtc = egdb_open("maxpieces=8", 100, DB_MTC, print_msgs);
	if (!mtc) {
		printf("Cannot open MTC db\n");
		exit(1);
	}
	fp = fopen(mtc_stats_filename, "r");
	if (!fp) {
		printf("Cannot open %s for reading\n", mtc_stats_filename);
		exit(1);
	}

	for (int count = 0; ; ) {
		if (!fgets(linebuf, ARRAY_SIZE(linebuf), fp))
			break;
		if (parse_fen(linebuf, &pos, &color))
			continue;

		/* For 7 and 8 pieces, only test every 10th position, to speed up the test. */
		++count;
		if ((count % (10 * FAST_TEST_MULT)) == 0 && bitcount64(pos.black | pos.white) >= 6) {
			print_fen(&pos, color, linebuf);
			printf("%s ", linebuf);
			value = mtc->lookup(mtc, (EGDB_POSITION *)&pos, color, 0);
			if (value >= 12) {
				test_best_mtc_successor(&wld, mtc, &pos, color);
			}

			printf("mtc %d\n", value);
		}
	}
	mtc->close(mtc);
	wld.handle->close(wld.handle);
}


void open_options_test(void)
{
	int value1, value2;
	EGDB_DRIVER *db;
	EGDB_POSITION pos;

	/* Verify that no slices with more than 2 kings on a side are loaded. */
	printf("\nTesting open options.\n");
	db = egdb_open("maxpieces=8;maxkings_1side_8pcs=2", 500, DB_TUN_V2, print_msgs);
	if (!db) {
		printf("Cannot open db at %s\n", DB_TUN_V2);
		exit(1);
	}
	indextoposition_slice(0, &pos, 2, 2, 2, 2);
	value1 = db->lookup(db, &pos, BLACK, 0);
	if (value1 != EGDB_WIN && value1 != EGDB_DRAW && value1 != EGDB_LOSS) {
		printf("bad value returned from lookup() in options test.\n");
		exit(1);
	}
	indextoposition_slice(0, &pos, 1, 3, 2, 2);
	value1 = db->lookup(db, &pos, BLACK, 0);
	value2 = db->lookup(db, &pos, WHITE, 0);
	if (value1 != EGDB_SUBDB_UNAVAILABLE || value2 != EGDB_SUBDB_UNAVAILABLE) {
		printf("bad value returned from lookup() in options test.\n");
		exit(1);
	}

	db->close(db);

	/* Verify that no slices with more than 6 pieces are loaded. */
	db = egdb_open("maxpieces=6", 500, DB_TUN_V2, print_msgs);
	if (!db) {
		printf("Cannot open db at %s\n", DB_TUN_V2);
		exit(1);
	}
	indextoposition_slice(0, &pos, 2, 1, 3, 0);
	value1 = db->lookup(db, &pos, BLACK, 0);
	if (value1 != EGDB_WIN && value1 != EGDB_DRAW && value1 != EGDB_LOSS) {
		printf("bad value returned from lookup() in options test.\n");
		exit(1);
	}
	indextoposition_slice(0, &pos, 4, 0, 3, 0);
	value1 = db->lookup(db, &pos, BLACK, 0);
	value2 = db->lookup(db, &pos, WHITE, 0);
	if (value1 != EGDB_SUBDB_UNAVAILABLE || value2 != EGDB_SUBDB_UNAVAILABLE) {
		printf("bad value returned from lookup() in options test.\n");
		exit(1);
	}

	db->close(db);
}


void crc_verify_test(char *dbpath)
{
	int abort;
	EGDB_DRIVER *db;
	EGDB_VERIFY_MSGS verify_msgs = {
		"crc failed",
		"ok",
		"errors",
		"no errors"
	};

	printf("\nTesting crc verify\n");
	db = egdb_open("maxpieces=6", 100, dbpath, print_msgs);
	if (!db) {
		printf("Cannot open db at %s\n", dbpath);
		exit(1);
	}
	abort = 0;
	db->verify(db, print_msgs, &abort, &verify_msgs);
	db->close(db);
}


int main(int argc, char* argv[])
{
	DB_INFO db1, db2;
	clock_t t0;
	SLICE slice;

	init_move_tables();

	if (identify(DB_TUN_V2, &db1))
		return(1);
	if (identify(DB_TUN_V1, &db2))
		return(1);

	/* Open the endgame db drivers. */
	db1.handle = egdb_open("maxpieces=7", 2000, DB_TUN_V2, print_msgs);
	if (!db1.handle) {
		printf("Cannot open tun v2 db\n");
		return(1);
	}
	db2.handle = egdb_open("maxpieces=7", 2000, DB_TUN_V1, print_msgs);
	if (!db2.handle) {
		printf("Cannot open tun v1 db\n");
		return(1);
	}

	/* Do a quick verification of the indexing functions. */
	t0 = clock();
	slice.reset();
	printf("\nTesting indexing round trip\n");
	while (1) {
		verify_indexing(&slice, 100000);
		if (slice.advance() > 9)
			break;
	}
	printf("%.2fsec: index test completed\n", TDIFF(t0));

	printf("\nVerifying WLD Tunstall v1 against Tunstall v2 (up to 7 pieces).\n");
	t0 = clock();
	slice.reset();
	while (1) {
		printf("%.2fsec: ", TDIFF(t0));
		verify(&db1, &db2, &slice, 50000 / FAST_TEST_MULT);
		if (slice.advance() > 7)
			break;
	}

	/* Close db2, leave db1 open for the next test. */
	db2.handle->close(db2.handle);

	printf("\nVerifying WLD runlen against WLD Tunstall v2 (up to 6 pieces).\n");
	if (identify(DB_RUNLEN, &db2))
		return(1);

	db2.handle = egdb_open("maxpieces=6", 2000, DB_RUNLEN, print_msgs);
	if (!db2.handle) {
		printf("Cannot open runlen db\n");
		return(1);
	}

	t0 = clock();
	slice.reset();
	while (1) {
		printf("%.2fsec: ", TDIFF(t0));
		verify(&db1, &db2, &slice, 50000 / FAST_TEST_MULT);
		if (slice.advance() > 6)
			break;
	}

	db1.handle->close(db1.handle);
	db2.handle->close(db2.handle);

	printf("\nSelf-verify Tunstall v2 test.\n");
	if (identify(DB_TUN_V2, &db1))
		return(1);

	db1.handle = egdb_open("maxpieces=8", 3000, DB_TUN_V2, print_msgs);
	if (!db1.handle) {
		printf("Cannot open tun v2 db\n");
		return(1);
	}

	EGDB_INFO db;
	db.dbpieces = db1.max_pieces;
	db.handle = db1.handle;
	db.dbpieces_1side = 5;
	db.egdb_excludes_some_nonside_caps = true;
	t0 = clock();
	slice.reset();
	while (1) {
		printf("%.2fsec: ", TDIFF(t0));
		self_verify(&db, &slice, 10000 / FAST_TEST_MULT);
		if (slice.advance() > 8)
			break;
	}
	db.handle->close(db.handle);
	mtc_test();
	open_options_test();

	crc_verify_test(DB_TUN_V1);
	crc_verify_test(DB_TUN_V2);
	return 0;
}

