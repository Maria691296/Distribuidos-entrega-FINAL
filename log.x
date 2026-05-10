program LOG {
    version LOGVER {
        int log_operation ( string user<256>, string operation<16>, string filename<256> ) = 1;
    } = 1;
} = 99;