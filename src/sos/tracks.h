const ushort Boot0[] = { 
    fset(29000,26000), 
    patch(PSAW1), 
    volume(10), 
    rest(194-50), 
    note(dd1,130), 
    rest(2), 
    note(dd1,125), 
    rest(3), 
    note(dd1,70), 
    rest(2), 
    note(dd1,77), 
    rest(3), 
    note(dd1,97), 
    rest(3), 
    note(dd1,91), 
    rest(3), 
    note(dd1,47), 
    rest(3), 
    note(dd1,51), 
    rest(3), 
    note(dd1,57), 
    rest(3), 
    note(dd1,132), 
    rest(0), 
    loop(255), 
    rest(20000), 
    endloop 
};
const ushort Boot1[] = { 
    fset(1000,26000), 
    patch(5), 
    volume(43), 
    note(cc2,1),

    loop(10), 
    finc(3000,26000), 
    slur(cc2,13), 
    endloop,

    ring(4), 
    volume(20),

    loop(60), 
    volume(1), 
    finc(-1000,26000), 
    slur(cc2,15), 
    endloop,

    loop(255), 
    rest(2000), 
    endloop 
};
const ushort Boot2[] = { 
    patch(PBUBBLE), 
    volume(12), 
    fset(6000,26000), 
    rest(134), 
    note(dd2,412+60),

    loop(20), 
    ring(20), 
    finc(1000,26000), 
    endloop,

    loop(30), 
    ring(10), 
    volume(4), 
    endloop,

    loop(255), 
    rest(20000), 
    endloop
};
const ushort Boot3[] = { 
    patch(PTHUNEL16),
    volume(40),
    fset(32767,26000),
    rest(134),
    note(dd3,820),
    volume(-25),
    note(dd3,200),

    loop(20),
    ring(20),
    volume(2),
    finc(-1000,26000),
    endloop,

    ring(10000),

    loop(255),
    rest(2000),
    endloop 
};
const ushort Boot4[] = { 
    rest(194),
    fset(32000,26000),
    patch(PNOISE1),
    volume(160),

    loop(8),
    volume(-3),
    note(cc4,15), 
    rest(5), 
    note(ff4,15), 
    volume(-3), 
    rest(5), 
    note(gg5,15), 
    rest(5), 
    volume(-3),
    note(ff4,15), 
    rest(5), 
    endloop, 

    loop(3), 
    xpose(0x100), 
    volume(-3), 
    note(cc4,10), 
    rest(5), 
    note(ff4,10), 
    volume(-3), 
    rest(5), 
    note(gg5,10), 
    rest(5), 
    volume(-3), 
    note(ff4,10), 
    rest(5), 
    endloop, 

    note(ff4,10), 

    loop(255), 
    rest(20000), 
    endloop 
};
const ushort Boot5[] = { 
    patch(PGLOCK), 
    volume(55), 
    rest(1114), 
    fset(32000,26000), 
    note(as2,20), 
    note(ff2,20), 
    note(as1,20), 
    volume(10),

    loop(6), 
    note(as2,18), 
    rest(2), 
    volume(30), 
    note(as2,18), 
    rest(2), 
    volume(20), 
    note(as2,18), 
    rest(2), 
    volume(-35), 
    finc(-2500,26000), 
    endloop,

    loop(255), 
    rest(20000), 
    endloop 
};
const ushort Boot6[] = { 
    loop(255), 
    rest(20000), 
    endloop
};
const ushort Boot7[] = { 
    patch(PSAW2), 
    xpose(0x60), 
    volume(18), 
    fset(1000,26000), 
    note(dd2,1), 

    loop(19), 
    slur(dd2,10), 
    finc(1500,2600), 
    endloop, 

    rest(800), 
    note(dd2,1), 

    loop(30), 
    slur(dd2,10), 
    finc(-750,2600), 
    endloop, 

    loop(30), 
    slur(dd2,5), 
    volume(3), 
    endloop, 

    loop(255), 
    rest(2000), 
    endloop
};
const ushort Boot8[] = { 
    loop(255), 
    rest(20000), 
    endloop
};
const ushort Boot9[] = { 
    patch(PTHUNEL16),
    volume(50), 
    rest(326-50), 
    note(aa2,280), 
    rest(0), 
    volume(-15), 
    note(aa2,244), 
    rest(0), 
    volume(-15), 
    note(aa2,500), 

    loop(255), 
    rest(2000), 
    endloop
};
const ushort BootA[] = { 
    patch(PTHUNEL16), 
    volume(50), 
    rest(454-50), 
    note(aa2,252), 
    rest(0), 
    note(aa2,194), 
    rest(0), 
    volume(-20), 
    note(aa2,200),

    loop(255), 
    rest(2000), 
    endloop
};
const ushort BootB[] = { 
    patch(PTHUNEL16), 
    volume(50), 
    rest(526-50), 
    note(aa2,274), 
    rest(0), 
    note(aa2,154), 
    rest(0), 
    volume(-20), 
    note(aa2,200),

    loop(255), 
    rest(2000), 
    endloop
};

struct TrackSource { const ushort* data; size_t count; };
#define TRACK_DEF(arr) { arr, sizeof(arr) / sizeof(arr[0]) }

const TrackSource BootSequence[MAX_TRACKS] = {
    TRACK_DEF(Boot0), 
    TRACK_DEF(Boot1), 
    TRACK_DEF(Boot2), 
    TRACK_DEF(Boot3),
    TRACK_DEF(Boot4), 
    TRACK_DEF(Boot5), 
    TRACK_DEF(Boot6), 
    TRACK_DEF(Boot7),
    TRACK_DEF(Boot8), 
    TRACK_DEF(Boot9), 
    TRACK_DEF(BootA), 
    TRACK_DEF(BootB)
};