# `egdb_intl`: a driver for international draughts endgame databases

`egdb_intl` is a set of C++ source files with functions to access the Kingsrow international draughts endgame databases. This package has been designed so that it can be easily integrated into existing C++ draughts programs. The source files are identical to those in the Kingsrow draughts program. 

The source code can be compiled for Windows using Microsoft Visual Studio 2015, and for Linux. 

The function to lookup the value of a position in the database is thread-safe and can be used in a multi-threaded search engine.

The code can be used to access 2 versions of the win, loss, draw (WLD) db, and the moves-to-conversion (MTC) db. These databases have information for positions with up to 8 pieces and with up to 5 pieces on one side.

From 2010 to 2014 I distributed version 1 of the databases on a portable external hard drive. This db is 407gb of data, which includes a small subset of 9-piece positions, 5 men vs. 4 men (I disabled the 9-piece functionality in this driver because I found through testing that on average it was less effective than not using it). In 2014 I created version 2 by re-compressing the db using better compression techniques, and excluding more positions to make the compression work better. Version 2 is 56gb of data, and is available for download from a file server. See link below.

You can contact me at eygilbert@gmail.com if you have questions or comments about these drivers.


# Obtaining the databases

The databases can be downloaded using the links at this web page: http://edgilbert.org/InternationalDraughts/endgame_database_downloads.htm

# Using the drivers

The primary functions for using the databases are `egdb_open()`, `egdb_lookup()`, and `egdb_close()`. The public interface to the databases are defined in `egdb_intl.h`. Include this file in any source file that needs to interface with the dbs. 

To avoid name collisions with user's public names, all declarations in `egdb_intl.h` are wrapped in a namespace named `egdb_interface`. You'll need to either include a `using namespace egdb_interface;` statement in your source files, or prefix any use of the driver interface names with the namespace name, e.g. `egdb_interface::egdb_open`. If you find the long namespace name annoying to type, you can define a short alias using something like 

    namespace dbi = egdb_interface; 
    
 and then use the alias instead of the full name.

To access a db for probing, it needs to be opened using the function `egdb_open()`. This returns a driver handle that contains function pointers for subsequent queries into the db.

Values in the db can be queried by calling through the `egdb_lookup()` function that takes a driver handle. Note that in general the databases do not have a value for every position with N pieces. Some positions are excluded to make the compression more effective, and you cannot always tell this from the value returned by `egdb_lookup()`, so you need to be aware of which positions are excluded and test for them before probing. See more below in the section "Excluded positions".

An opened driver can also be queried for several attributes of the database, such as the maximum number of draughts pieces it has data for. Another function does a crc verification of all the db files.

When a user is finished with the driver, it can be closed to free up resources. The `egdb_close()` function is accessed through another function pointer in the driver handle.

# Example program

The driver sources include an example program in the directory `example`. It opens a database, reads some positions from a table, calls `egdb_lookup()` to get the database value of each position, and compares it to the known value that is also in the table. You can use this to verify that basic functions of the driver are working on your system.

The example includes a project file for Microsoft Visual Studio 2015 (for which there is a free Community Edition), and a CMake script that will generate a Linux Makefile. To run the example, edit the macro `DB_PATH` near the top of `example/main.cpp` to point it to the location of your db files, then compile and run without arguments. On Windows, this typically is `C:/Program Files/Kingsrow International/wld_database` or `C:/Program Files/Kingsrow International/mtc_database`. On dual-boot or virtual machine Linux installations, one can mount these paths under `/media/` followed by a `sudo usermod -a -G vboxsf USERNAME`

To compile on Linux, do the usual CMake incantations for an out-of-tree build:

    cd egdbl_intl
    mkdir build
    cd build
    cmake ..
    make

When the build finishes, you can run `example.main` from the `build` directory by calling

    ./example/example.main

# Excluded positions

Version 1 of the WLD db is identified as type EGDB_WLD_TUN_V1. Version 1 filenames have suffixes ".cpr" and ".idx". This db does not have valid data for any position that is a capture for the side-to-move. There is no way to know this from the lookup return value. It will be a win, loss, or draw value, but it will not be the correct value except by random luck. It will be whatever makes the compression work best. Before calling lookup, you should first test that the position is not a capture.

Version 2 of the WLD db is identified as type EGDB_WLD_TUN_V2. Version 2 filenames have suffixes ".cpr1" and ".idx1". Like version 1, it does not have valid data for positions that are a capture for the side-to-move. Additionally, for positions with 7 and 8 pieces that have more than 1 king present, it does not have valid data for positions that would be captures for the opposite of the side-to-move (non-side captures), and it only has data for one side. Which side is excluded varies with position. It's not always black or always white, although it is consistent within each slice. It's whatever makes the compression more effective. If you call lookup and the data for that side is not present, you get the return EGDB_UNKNOWN. If you call lookup for a position that is excluded because of a capture or non-side capture, you will get a WLD value, but it will not be the correct value except by random luck. The value will be whatever makes the compression work best.

The MTC db is identified as type EGDB_MTC_RUNLEN. It has filenames with suffixes ".cpr_mtc" and ".idx_mtc". This db has data for both side-to-move colors, and for positions with non-side captures. To make the db considerably smaller, it does not have data for positions where the distance to a conversion move is less than 10 plies. The return from lookup for such positions is MTC_LESS_THAN_THRESHOLD. Draughts engines typically do not need databases for such positions because a search can usually provide a good move. The db only has valid data for positions that are a win or loss. You cannot tell this from the return value of a lookup in the MTC db, so before querying an MTC value you should use a WLD db to determine that the position is a win or a loss. 

# Reference documentation

## Types

### `egdb_interface::EGDB_TYPE`
    enum EGDB_TYPE {
        EGDB_WLD_RUNLEN = 0,
        EGDB_MTC_RUNLEN,
        EGDB_WLD_HUFFMAN,
        EGDB_WLD_TUN_V1,
        EGDB_WLD_TUN_V2,
    };

**Notes**: there is a single version of the MTC-database, and several versions of WLD-databases, each using a different compression technique:
  - `EGDB_WLD_RUNLEN`: [runlength encoding](https://en.wikipedia.org/wiki/Run-length_encoding). This original format  was only distributed to a limited number of Kingsrow testers, and is now deprecated.
  - `EGDB_MTC_RUNLEN`: runlength encoding. There are no other formats for the MTC-datbase.
  - `EGDB_WLD_HUFFMAN`: [Huffman coding](https://en.wikipedia.org/wiki/Huffman_coding). This experimental format used substantially less disk space at the expense of substantially more time during the engine search. For this reason, this version was never released.  
  - `EGDB_WLD_TUN_V1`: a mixture of [Tunstall coding](https://en.wikipedia.org/wiki/Tunstall_coding) and runlength encoding. The databases are 407gb of data, which includes a small subset of 9-piece positions: 5 men vs. 4 men. However, the 9-piece functionality is disabled in this driver because on average it turns out to be less effective during an engine search than not using it. From 2010 to 2014, these databases were bundled with the commercially available Kingsrow program, and distributed on a portable external hard drive. See also [this thread](http://laatste.info/bb3/viewtopic.php?t=2905) on the FMJD forum.
  - `EGDB_WLD_TUN_V2`: an optimized version of Tunstall coding, and also excluding more positions. The databases are 56gb of data. From 2014 onwards, these databases have been available for download from a file server. 

---

### `egdb_interface::EGDB_BITBOARD`
    #include <cstdint>    
    typedef uint64_t EGDB_BITBOARD;
    
**Notes**: For 10x10 board representations, bits 0 through 53 represent squares 1 through 50, with gaps (sometimes called "ghost squares") at bits 10, 21, 32, and 43, and bits 54 through 63. See also [this thread](http://laatste.info/bb3/viewtopic.php?f=53&t=1925&start=60) on the FMJD forum.   

---

### `egdb_interface::EGDB_POSITION`    
    struct EGDB_POSITION {
        EGDB_BITBOARD black;
        EGDB_BITBOARD white;
        EGDB_BITBOARD king;
    };
    
**Notes**: a bitboard representation of an international draughts position containing the black pieces, white pieces and kings, respectively. 

**Invariants**: In every legal 10x10 draughts position, the following constraints are satisfied:
   - the `black` and `white` bitboards have an empty intersection (bitwise-and); 
   - every piece in the `kings` bitboard is also contained within the union (bitwise-or) of the `black` and `white` bitboards;
   - any `white` piece located on squares 1 through 5, and any `black` piece located on squares 46 through 50, is also present in the `kings` bitboard.       
    
---

### `egdb_interface::EGDB_COLOR`    
    enum EGDB_COLOR {
        EGDB_BLACK = 0,
        EGDB_WHITE = 1
    };

---

### `egdb_interface::EGDB_STATS`    
    struct EGDB_STATS {
        unsigned int lru_cache_hits;
        unsigned int lru_cache_loads;
        unsigned int autoload_hits;
        unsigned int db_requests;               /* total egdb requests. */
        unsigned int db_returns;                /* total egdb w/l/d returns. */
        unsigned int db_not_present_requests;   /* requests for positions not in the db */
        float avg_ht_list_length;
    };

---

### `egdb_interface::EGDB_VERIFY_MSGS`
    struct EGDB_VERIFY_MSGS {
        char crc_failed[80];
        char ok[80];
        char errors[80];
        char no_errors[80];
    };

---

### `egdb_interface::EGDB_DRIVER`
    struct EGDB_DRIVER {
        // implementation defined
    };
    
**Notes** This is an opaque class whose implementation is not documented, and should not be relied upon. All calls of the form `handle->some_function(handle, other_args)` are now deprecated and support for them can be removed in a future release. Instead, use the new form `egdb_some_function(handle, other_args)`. In general, variables of type `EGDB_DRIVER` should only be accessed through global functions such as `egdb_open()`, `egdb_lookup()` and `egdb_close()` documented below. Note that this is similar to how `std::FILE` should only be accessed through the `std::fopen`, `std::fread` and `std::fclose` functions.    
    
---

## Global functions

### `egdb_interface::egdb_identify`
    int egdb_identify(
        char const *directory, 
        EGDB_TYPE *egdb_type, 
        int *max_pieces
    );

**Effects**: Checks if an endgame database exists in `directory`. If so, the identified endgame database type is written into `egdb_type` and the maximum number of pieces for any of the databases identified is written into `max_pieces`. Otherwise, these out-parameters are unchanged on return.

**Returns**: Zero if a database is found, non-zero otherwise.

---

### `egdb_interface::egdb_open`
    EGDB_DRIVER *egdb_open(
        char const *options, 
        int cache_mb, 
        char const *directory, 
        void (*msg_fn)(char const *msg)
    );

**Parameters**: 
  - `options`:  a string of optional open settings. The options are of the form `name = value`. Multiple options are separated by a semicolon. Either a `NULL` pointer or an empty string ("") can be given for no options. The options were more useful a few years ago when computers had less memory. Now they are not usually needed. These options are defined:
    - `maxpieces = N`: sets the maximum number of pieces for which the driver will lookup values. By default the driver will use all the database files that it finds during initialization. If you restrict the number of pieces, the driver will not have to load indexing data for larger positions, saving time during initialization and using its allotted memory more efficiently.
    - `maxkings_1side_8pcs = N` restricts the set of 8-piece positions for which the driver will lookup values. This saves time and memory during initialization.
  - `cache_mb`: the number of MiB (`2^20` bytes) of dynamically allocated memory that the driver will use. 
  - `directory`: the full path to the location of the database files.  
  - `msg_fn`: a function pointer that will receive status and error messages from the driver. 

**Effects**: opens a database, allocates memory for caching db values, and reads indexing files. The driver normally emits status messages when it starts up, with information about files opened and memory used. If any errors occur, either during the `egdb_open()` call or in subsequent calls through the driver `handle`, the details will be reported through the message function. A message function that writes messages to stdout can be defined like this:

    void msg_fn(char const *msg)
    {
        printf("%s", msg);
    }

In Kingsrow I save driver messages to a log file. It can be useful when diagnosing an unexpected problem. 

**Returns**: a pointer to a structure that has function pointers for subsequent communication with the driver. A `nullptr` return value means that an error occurred. 

**Complexity**: linear in the size of the databases to be read in from disk.

**Notes**: Opening a db takes some time, but allows the db to be probed quickly during an engine search.

Some of it is used for indexing data, and the rest is used to dynamically cache database data to minimize disk access during lookups. The more memory you give the driver, the faster it works on average during an engine search. On a machine with at least 8gb of ram, setting the cache_mb to something like 3000mb less than the total PC memory gives good driver performance and still leaves some memory for Windows drivers and a few other smaller programs to run. On machines with less memory you'll have to make the margin smaller and manage the memory usage more carefully. My experience is that the driver actually works surprisingly well in a Kingsrow search with the 8pc db and a very small setting like 1500mb, but of course more memory is better.

If you are opening the MTC db, give it a cache_mb value of 0. It will then use a relatively small amount of memory, approximately 25mb, which is all it needs. Unlike the WLD db, the MTC db is not used during a search. It is probed before a search, and if a move can be obtained from the MTC db then a search is not necessary. To obtain a move from the MTC db it is only necessary to probe the MTC values of the immediate successors of the target position and pick a successor with the best MTC value -- the lowest value for a won position, or the highest value for a lost position. When probing successor values, it is necessary to skip probing of successors that were conversion moves from the target position. A conversion move is a capture or a man move.

---

### `egdb_interface::egdb_lookup`
    int egdb_lookup(
        EGDB_DRIVER *handle, 
        EGDB_POSITION /*const*/ *position, 
        int color, 
        int cl
    );

**Parameters**:
  - `handle`: a value returned from `egdb_open()`.
  - `position`: a legal 10x10 international draughts position. This position is not modified by `lookup()`. However, for legacy reasons, `position` is not declared as `EGDB_POSITION const*`. 
  - `color`: the side-to-move, either `EGDB_BLACK` or `EGDB_WHITE`.
  - `cl`: conditional lookup. If it is non-zero, then the driver will only get the value of the position if the data is already cached in ram, otherwise `EGDB_NOT_IN_CACHE` will be returned. If the conditional lookup argument is zero, the driver will always attempt to get a value for the position even if it has to read a disk file to get it. 

**Returns**: a database value. 

For the WLD databases, the values returned by `lookup()` are typically defined by one of the macros `EGDB_WIN`, `EGDB_LOSS`, or `EGDB_DRAW`. 

If the query is of a subset that has incompete data, such as 5men vs. 4men, lookup might also return `EGDB_UNKNOWN`, `EGDB_DRAW_OR_LOSS`, or `EGDB_WIN_OR_DRAW`. 

If you query a side-to-move of a 7pc or 8pc that is excluded from the database, you will get `EGDB_UNKNOWN`. 

If you had set the conditional lookup arguement true, you might get the return `EGDB_NOT_IN_CACHE`. 

If for example you query an 8pc position, but you only opened the driver for a maximum of 7 pieces, then you will get the value `EGDB_SUBDB_UNAVAILABLE`. 

Note that depending on the position and the database type, you sometimes cannot call lookup for non-side capture positions, and you can never call lookup for capture positions, because the value returned will not be correct except by random luck. See the section "Excluded positions" for more details.

For the MTC databases, the values returned by `egdb_lookup()` are either the number of plies to a conversion move, or the value `MTC_LESS_THAN_THRESHOLD` for positions that are close to a conversion move. The mtc databases do not store positions that are closer than `MTC_THRESHOLD` plies to a conversion move. It will only return even mtc values, because the database represents the mtc value internally by `distance/2`. The true value is either the value returned, or `(value-1)`. An application program can infer the true even or odd value of a position by looking up the mtc values of the position's successors. If the best successor mtc value is the same as the position's, then the position's true value is 1 less than the returned value.

**Notes**: An example of a place to use conditional lookup is during quiescence search, where you don't want the search to be slowed by disk access. If the database is not on a fast SSD you might want to use conditional lookup in some other areas of a search to limit the slowing.

---

### `egdb_interface::egdb_reset_stats`
    void egdb_reset_stats(
        EGDB_DRIVER *handle
    );
    
### `egdb_interface::egdb_get_stats`
    EGDB_STATS *egdb_get_stats(
        EGDB_DRIVER *handle
    );
    
**Notes**: These functions for collecting statistics about the database use are primarily for use by the driver developer and are deprecated in this public release of the driver. They may be removed in future releases.

---

### `egdb_interface::egdb_verify`
    int egdb_verify(
        EGDB_DRIVER *handle, 
        void (*msg_fn)(char const *msg), 
        int *abort, 
        EGDB_VERIFY_MSGS *msgs
    );

**Parameters**: 
  - `abort`: if non-zero, cancels the verification process (because it can take a while). To abort verification, set the value of *abort to non-zero. 
  - `msgs`: an `EGDB_VERIFY_MSGS` struct of language localization messages used by verify.

**Effects**: checks that every db index and data file has a correct CRC value. 

**Returns**: zero if all CRCs compared ok, non-zero if there are any CRCs that do not match, a non-zero value is returned, and an error message is written through the `msg_fn`. 

---

### `egdb_interface::egdb_close`
    int egdb_close(
        EGDB_DRIVER *handle
    );

**Effects**: closes the driver and frees resources. 

**Returns**: zero on success, non-zero if there are any errors.

**Notes**: If the application needs to change anything about an egdb driver after it has been opened, such as number of pieces or megabytes of ram to use, it must be closed and then opened again with the new parameters.  

---

### `egdb_interface::egdb_get_pieces`
    int egdb_get_pieces(
        EGDB_DRIVER *handle, 
        int *max_pieces, 
        int *max_pieces_1side, 
        int *max_9pc_kings, 
        int *max_8pc_kings_1side
    );

`"get_pieces"` is a way to query some attributes of an open database.

---

# Acknowledgements

Rein Halbersma ported the code to Linux and improved it in other ways also. Thanks Rein!

# License

    Original work Copyright (C) 2007-2016 Ed Gilbert
    Linux port Copyright (C) 2016 Rein Halbersma and Ed Gilbert

Distributed under the Boost Software License, Version 1.0.
(See accompanying file `LICENSE_1_0.txt` or copy at
[http://www.boost.org/LICENSE_1_0.txt](http://www.boost.org/LICENSE_1_0.txt))
