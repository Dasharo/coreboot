Files in this directory come from talos-hostboot repo, commit a2ddbf3 [1]. No
changes were made, other than converting '#include <...>' to '#include "..."'.

In some cases units mentioned in comments in the bigger structure are different
than in comments above internal structure definitions and field names. An
example of such difference is VpdOperatingPoint, defined in p9_pstates_common.h,
where "voltages are specified in units of 1mV, and characterization currents are
specified in units of 100mA", which is consistent with its fields names. When
this structure is used in other structures (each other file uses it), unit for
voltage becomes 5mV, and for currents - 500mA.

Another issue is poundW_data - it doesn't have 'packed' attribute, but it is
packed in MVPD. Additional fields were added at some point [2], with a comment
that additional reserved field was added to keep the size the same. The problem
is that new field (the important one, not reserved) was added in the middle of
the structure, between uint64 that is by default naturally aligned to 8B, and
that is the biggest alignment used in that structure. Modifying anything after
that field won't help in keeping the size the same as before. Luckily, offsets
to all of the non-reserved fields are proper. sizeof() or an array of this type
cannot be used.

There may be other inconsistencies, be advised.

[1] https://git.raptorcs.com/git/talos-hostboot/tree/src/import/chips/p9/procedures/hwp/lib?id=a2ddbf3150e2c02ccc904b25d6650c9932a8a841
[2] https://git.raptorcs.com/git/talos-hostboot/commit/src/import/chips/p9/procedures/hwp/lib/p9_pstates_cmeqm.h?id=2ab88987e5fed942b71b757e0c2972adee5b8e1b
