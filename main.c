#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lodepng.h"

typedef struct Node {
    unsigned char r, g, b, w;
    struct Node *up, *down, *left, *right, *parent;
    int rank;
} Node;

typedef struct point {
    int x, y;
} point;

char* load_png_file(const char *filename, int *width, int *height) {
    unsigned char *image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error) {
        printf("error: can't load the picture %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    return (image);
}

Node* create_graph(const char *filename, int *width, int *height)
{
    unsigned char *picture = NULL;
    int error = lodepng_decode32_file(&picture, width, height, filename);
    printf("%d", error);
    if(error) {
        printf("error: can't read the picture %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    Node* nodes = malloc(*width * *height * sizeof(Node));
    for(unsigned j = 0; j < *height; ++j) {
        for (unsigned i = 0; i < *width; ++i) {
            Node* node = &nodes[j * *width + i];
            unsigned char* pixel = &picture[(j * *width + i) * 4];
            node->r = pixel[0];
            node->g = pixel[1];
            node->b = pixel[2];
            node->w = pixel[3];
            if(j > 0) {
                node->up = &nodes[(j - 1) * *width + i];
            }
            else {
                node->up = NULL;
            }
            if(j < *height - 1) {
                node->down = &nodes[(j + 1) * *width + i];
            }
            else {
                node->down = NULL;
            }
            if(i > 0) {
                node->left = &nodes[(j * *width + (i - 1))];
            }
            else {
                node->left = NULL;
            }
            if(i < *width - 1) {
                node->right = &nodes[(j * *width + (i + 1))];
            }
            else {
                node->right = NULL;
            }
            node->parent = node;
            node->rank = 0;
        }
    }

    free(picture);
    return nodes;
}

Node* find_set(Node* x)
{
    if(x->parent != x) {
        x->parent = find_set(x->parent);
    }
    return x->parent;
}

void union_set(Node* x, Node* y, double epsilon) {
    if (x->r < 40 && y->r < 40) {
        return;
    }
    Node* px = find_set(x);
    Node* py = find_set(y);

    double difference = sqrt(pow(x->r - y->r, 2) + pow(x->g - y->g, 2) + pow(x->b - y->b, 2));
    if(px != py && difference < epsilon) {
        if (px->rank > py->rank) {
            py->parent = px;
        } 
        else {
            px->parent = py;
            if(px->rank == py->rank) {
                py->rank++;
            }
        }
    }
    return;
}

void apply_filter(unsigned char *image, int width, int height) {

    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}}; 
    int i, j;
    unsigned char *result = malloc(width * height * 4 * sizeof(unsigned char));
    for(j = 1; j < height - 1; j++) {
        for(i = 1; i < width - 1; i++) {
            int sumX = 0;
            int sumY = 0;
            for(int dy = -1; dy <= 1; dy++) {
                for(int dx = -1; dx <= 1; dx++) {
                    int index;
                    index = ((j + dy) * width + (i + dx)) * 4;
                    int red = (image[index] + image[index + 1] + image[index + 2]) / 3;
                    sumX += gx[dy + 1][dx + 1] * red;
                    sumY += gy[dy + 1][dx + 1] * red;
                }
            }
            int grad_len;
            grad_len = sqrt(sumX * sumX + sumY * sumY);
            if(grad_len > 255) {
                grad_len = 255;
            }
            int res = (j * width + i) * 4; 
            result[res] = (unsigned char)grad_len;
            result[res + 1] = (unsigned char)grad_len;
            result[res + 2] = (unsigned char)grad_len;
            result[res + 3] = image[res + 3]; 
        }
    }
    for(int k = 0; k < width * height * 4; k++) {
        image[k] = result[k];
    }
    free(result);
    return;
}

void find_components(Node* nodes, int width, int height, double epsilon) {
    int i, j;
    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            Node* node = &nodes[j * width + i];
            if(node->up) {
                union_set(node, node->up, epsilon);
            }
            if(node->down) {
                union_set(node, node->down, epsilon);
            }
            if(node->left) {
                union_set(node, node->left, epsilon);
            }
            if(node->right) {
                union_set(node, node->right, epsilon);
            }
        }
    }
    return;
}

void flood_fill(unsigned char* image, int x, int y, int new_color1, int new_color2, int new_color3, int old_color, int width, int height) {
    int dx[] = {-1, 0, 1, 0};
    int dy[] = {0, 1, 0, -1};
    point* stack = malloc(width * height * 4 * sizeof(point));
    long top = 0;

    stack[top++] = (point){x, y};

    while(top > 0) {
        point p = stack[--top];

        if(p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) {
            continue;
        }
        int ind = (p.y * width + p.x) * 4;
        if(image[ind] > old_color) {
            continue;
        }
        image[ind] = new_color1;
        image[ind + 1] = new_color2;
        image[ind + 2] = new_color3;

        for(int i = 0; i < 4; i++) {
            int n_x = p.x + dx[i];
            int n_y = p.y + dy[i];
            int n_ind = (n_y * width + n_x) * 4;
            if(n_x > 0 && n_x < width && n_y > 0 && n_y < height && image[n_ind] <= old_color) {
                stack[top++] = (point){n_x, n_y};
            }
        }
    }
    free(stack);
}


void color_components(unsigned char* image, int width, int height, int epsilon) {
    int color1 = epsilon * 2;
    int color2 = epsilon * 2;
    int color3 = epsilon * 2;
    int i, j;
    for(j = 1; j < height - 1; j++) {
        for(i = 1; i < width - 1; i++) {
            if(image[4 * (j * width + i)] < epsilon) {
                flood_fill(image, i, j, color1, color2, color3, epsilon, width, height);

            }
            color1 = rand() % (255 - epsilon * 2) + epsilon * 2;
            color2 = rand() % (255 - epsilon * 2) + epsilon * 2;
            color3 = rand() % (255 - epsilon * 2) + epsilon * 2;
        }
    }
}



int main() {
    int w = 0;
    int h = 0; 
    char *filename = "skull_input.png";
    char *picture = load_png_file(filename, &w, &h);
     double epsilon = 25.0;
    apply_filter(picture, w, h);
    char *output_filename = "output.png";

    Node* nodes = create_graph(filename, &w, &h);

    find_components(nodes, w, h, epsilon);
    color_components(picture, w, h, epsilon);
    lodepng_encode32_file(output_filename, picture, w, h);
    free(picture);

    return 0;
}