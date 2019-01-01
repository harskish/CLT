kernel void power(global int* input, global int* output) {
    uint gid = get_global_id(0);
    if (gid >= N)
        return;

    output[gid] = pow(input[gid], (float)P);
}
