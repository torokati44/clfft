
kernel void fill_cos_sin(global float2 *cos_sin_buffer) {
    int id = get_global_id(0);
    float t = (float)id / get_global_size(0);
    float cost;
    float sint = sincos(t * 3.1415926538f * 2.0f, &cost);
    cos_sin_buffer[id].x = cost;
    cos_sin_buffer[id].y = sint;
}

kernel void increment_indices(global int *index_buffer) {
    int id = get_global_id(0);
    index_buffer[id] += id;
    index_buffer[id] %= get_global_size(0) * 2;
}

kernel void add_sample(
        float sample,
        float alpha,
        global int *index_buffer,
        global float4 *spectrum_buffer,
        global float2 *cos_sin_buffer) {
    int id = get_global_id(0);

    float2 product = cos_sin_buffer[index_buffer[id]] * sample;
    float2 element = spectrum_buffer[id].xy * alpha + product * (1.0f - alpha);

    float len = length(element.xy);
    float ang = atan2(element.y, element.x);

    spectrum_buffer[id] = (float4)(element, len, ang);
}
