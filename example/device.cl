kernel void power(global int* input, global int* output) {
    uint gid = get_global_id(0);
    if (gid >= LEN)
        return;

    output[gid] = pow(input[gid], (float)POWR);
}
