#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

std::vector<int> createRandomPermutation() {
    // Create a vector with integers from 0 to 255
    std::vector<int> permutation(256);
    for (int i = 0; i < 256; ++i) {
        permutation[i] = i;
    }

    // Use random_device for a random seed
    std::random_device rd;
    std::mt19937 g(rd());

    // Shuffle the permutation vector
    std::shuffle(permutation.begin(), permutation.end(), g);

    // Duplicate the permutation vector
    permutation.insert(permutation.end(), permutation.begin(), permutation.end());
    
    return permutation;
}

std::vector<int> getPermutationVector () {
    std::vector<int> p;

    std::vector<int> permutation = createRandomPermutation();

    p.insert(p.end(), permutation.begin(), permutation.end());
    p.insert(p.end(), permutation.begin(), permutation.end());
    
    return p;
}

double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}
    
double lerp(double t, double a, double b) {
    return a + t * (b - a);
}

double grad(int hash, double x, double y, double z) {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}
    
double perlinNoise(float x, float y, std::vector<int> &p) {
    int z = 0.5;
    
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    double u = fade(x);
    double v = fade(y);
    double w = fade(z);
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                   grad(p[BA], x - 1, y, z)),
                           lerp(u, grad(p[AB], x, y - 1, z),
                                   grad(p[BB], x - 1, y - 1, z))),
                   lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                   grad(p[BA + 1], x - 1, y, z - 1)),
                           lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                   grad(p[BB + 1], x - 1, y - 1, z - 1))));
}