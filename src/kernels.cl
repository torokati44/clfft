
kernel void fill_cos_sin(global float2 *cos_sin_buffer) {
    int id = get_global_id(0);
    float t = (float)id / get_global_size(0);
    float cost;
    float sint = sincos(t * 3.1415926536f * 2.0f, &cost);
    cos_sin_buffer[id].x = cost;
    cos_sin_buffer[id].y = sint;
}

kernel void add_samples(
        int num_samples,
        global float *samples,
        float alpha,
        global int *index_buffer,
        global float4 *spectrum_buffer,
        global float2 *cos_sin_buffer) {
    int id = get_global_id(0);
    int sample_rate = get_global_size(0) * 2;

    float beta = 1.0f - alpha;
    float2 element = spectrum_buffer[id].xy;
    int index = index_buffer[id];

    for (int i = 0; i < num_samples; ++i) {
        float2 product = cos_sin_buffer[index] * samples[i];
        //element *= alpha;
        element += product;// * beta;

        index += id;
        index %= sample_rate;

        element *= alpha;
    }

    index_buffer[id] = index;
    float len = length(element.xy);
    float ang = atan2(element.y, element.x);

    spectrum_buffer[id] = (float4)(element, len, ang);
}
