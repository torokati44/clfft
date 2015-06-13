
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




float2 cplx_mul(float2 a, float2 b) {
    return (float2)(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

kernel void transform_image(
        int size,
        global float2 *input,
        global float2 *output) {
    int id = get_global_id(0);
    int x = id % size;
    int y = id / size;

    float2 value = (float2)(0);
    float2 cos_sin;

    for (int iy = 0; iy < size; ++iy) {
        for (int ix = 0; ix < size; ++ix) {
            float t = (ix * x + iy * y) / (float)size * 3.14159265f * 2.0f;
            cos_sin.x = cos(t);
            cos_sin.y = sin(t);

            value += cplx_mul(input[ix + size * iy], cos_sin);
        }
    }

    output[id] = value / size;
}

float butterworth(float t, int n) {
    return sqrt(1.0f/(1.0f + pow(t, 2*n)));
}

kernel void inverse_transform_image(
        int size,
        global float2 *input,
        global float2 *output) {
    int id = get_global_id(0);
    int x = id % size;
    int y = id / size;

    float2 value = (float2)(0);
    float2 cos_sin;

    for (int iy = 0; iy < size; ++iy) {
        for (int ix = 0; ix < size; ++ix) {
            float t = (ix * x + iy * y) / (float)size * 3.14159265f * 2.0f;
            cos_sin.x = cos(t);
            cos_sin.y = sin(t);

            value += input[ix + size * iy] * cos_sin;
        }
    }

    output[id] = value / size;
}


kernel void filter_bandpass(
        int size,
        global float2 *image_transformed
) {
    int id = get_global_id(0);
    int x = id % size;
    int y = id / size;

    float coeff = butterworth(length((float2)(x, y))/10 - 5.0f, 2);

    image_transformed[id] *= coeff;
}

kernel void filter_multiply(
        int image_size,
        global float2 *image_in,
        global float2 *image_out,
        int spectrum_length,
        global float4 *spectrum
) {
    int id = get_global_id(0);
    int x = id % image_size;
    int y = id / image_size;

    float t = min(length((float2)(x, y)), length((float2)(image_size-x-1, image_size-y-1))) / length((float2)(image_size/2, image_size/2));

    float coeff = spectrum[(int)(spectrum_length*t/2.0f)].z*sqrt(t);
    image_out[id] = image_in[id] * coeff;
}

