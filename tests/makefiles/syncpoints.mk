# A .SYNC dependent that isn't a target of the makefile has no description block.
# Translating the sync points must skip those instead of stopping, so every t<n>
# below has to end up depending on the m<n>.txt in front of it. The sync points are
# iterated in QHash order, hence the number of pairs.

all: m0.txt .SYNC t0 .SYNC m1.txt .SYNC t1 .SYNC m2.txt .SYNC t2 \
     .SYNC m3.txt .SYNC t3 .SYNC m4.txt .SYNC t4

t0:
t1:
t2:
t3:
t4:
