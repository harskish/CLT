#ifndef N
#define N 0
#endif

kernel void square(global int* input, global int* output) {
    uint gid = get_global_id(0);
    if (gid >= N)
        return;

    output[gid] = input[gid] * input[gid];
}