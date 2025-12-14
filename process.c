#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

/* The RGB values of a pixel. */
struct Pixel
{
    // hexadecimal values
    // Represents a pixel with three 16-bit color channels
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};

/* An image loaded from a file. */
struct Image
{
    int height;
    int width;
    struct Pixel *pixels; // Pointer to an array of pixels (dynamically allocated).
};

/* Free a struct Image */
void free_image(struct Image *img)
{
    if (img)
    {
        free(img->pixels);
        free(img);
    }
}

/* Opens and reads an image file, returning a pointer to a new struct Image.
 * On error, prints an error message and returns NULL. */
struct Image *load_image(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL)
    {
        fprintf(stderr, "File %s could not be opened.\n", filename);
        return NULL;
    }

    struct Image *img = malloc(sizeof(struct Image));// Allocate memory for the image struct
    if (img == NULL)
    {
        fclose(f);
        return NULL;
    }

    if (fscanf(f, "HPHEX %d %d ", &img->height, &img->width) != 2)
    {
        fprintf(stderr, "Error reading image dimensions from %s.\n", filename);
        free(img);
        fclose(f);
        return NULL;
    }

    img->pixels = malloc(img->height * img->width * sizeof(struct Pixel));
    if (img->pixels == NULL)
    {
        free(img);
        fclose(f);
        return NULL;
    }

    //Reading pixel data from the file and store it in the pixels array
    for (int i = 0; i < img->height * img->width; i++)
    {
        if (fscanf(f, "%4hx %4hx %4hx ", &img->pixels[i].red, &img->pixels[i].green, &img->pixels[i].blue) != 3)
        {
            fprintf(stderr, "Error reading pixel data from %s.\n", filename);
            free_image(img);
            fclose(f);
            return NULL;
        }
    }

    fclose(f);
    return img;
}

/* Write img to file filename. Return true on success, false on error. */
bool save_image(const struct Image *img, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        return false;
    }

    //Write pixel data to the file
    fprintf(f, "HPHEX %d %d ", img->height, img->width);
    for (int i = 0; i < img->height * img->width; i++)
    {
        fprintf(f, "%04x %04x %04x ", img->pixels[i].red, img->pixels[i].green, img->pixels[i].blue);
    }

    fclose(f);
    return true;
}

/* Allocate a new struct Image and copy an existing struct Image's contents
 * into it. On error, returns NULL. */
struct Image *copy_image(const struct Image *source)
{
    if (source == NULL)
    {
        return NULL;
    }

    struct Image *copy = malloc(sizeof(struct Image));
    if (copy == NULL)
    {
        return NULL;
    }

    copy->height = source->height;
    copy->width = source->width;

    copy->pixels = malloc(copy->height * copy->width * sizeof(struct Pixel));
    if (copy->pixels == NULL)
    {
        free(copy);
        return NULL;
    }

    //Copy the pixel data from the source image to the new image
    memcpy(copy->pixels, source->pixels, copy->height * copy->width * sizeof(struct Pixel));
    return copy;
}

/* Returns a new struct Image containing the result, or NULL on error. */
struct Image *apply_BLUR(const struct Image *source)
{
    if (source == NULL)
    {
        return NULL;
    }

    struct Image *output = copy_image(source);
    if (output == NULL)
    {
        return NULL;
    }

    //Looping through each pixel in the image
    for (int y = 0; y < source->height; y++)
    {
        for (int x = 0; x < source->width; x++)
        {
            int red = 0, green = 0, blue = 0;
            int count = 0;

            // Loop through the 3x3 block around the current pixel.
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (ny >= 0 && ny < source->height && nx >= 0 && nx < source->width)
                    {
                        int index = ny * source->width + nx;
                        red += source->pixels[index].red;
                        green += source->pixels[index].green;
                        blue += source->pixels[index].blue;
                        count++;
                    }
                }
            }
            int index = y * source->width + x;
            output->pixels[index].red = red / count;
            output->pixels[index].green = green / count;
            output->pixels[index].blue = blue / count;
        }
    }
    return output;
}

/* Returns true on success, or false on error. */
bool apply_NORM(struct Image *img)
{
    if (img == NULL)
    {
        return false;
    }

    int min_val = INT_MAX, max_val = INT_MIN;

    // Loop through each pixel in the image to find the min and max color values.
    for (int i = 0; i < img->height * img->width; i++)
    {
        struct Pixel *pix = &img->pixels[i];
        if (pix->red > max_val)
            max_val = pix->red;
        if (pix->green > max_val)
            max_val = pix->green;
        if (pix->blue > max_val)
            max_val = pix->blue;
        if (pix->red < min_val)
            min_val = pix->red;
        if (pix->green < min_val)
            min_val = pix->green;
        if (pix->blue < min_val)
            min_val = pix->blue;
    }

    if (max_val == min_val)
    {
        fprintf(stderr, "Image is already normalised.\n");
        return true;
    }

    float scale =(float) 65535 / (max_val - min_val);

    printf("Minimum value: %d\nMaximum value: %d\n", min_val, max_val);

    for (int i = 0; i < img->height * img->width; i++)
    {
        img->pixels[i].red = (img->pixels[i].red - min_val) * scale;
        img->pixels[i].green = (img->pixels[i].green - min_val) * scale;
        img->pixels[i].blue =  (img->pixels[i].blue - min_val) * scale;
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: process INPUTFILE1 OUTPUTFILE1 [INPUTFILE2 OUTPUTFILE2 ...]\n");
        return 1;
    }

    int num_images = (argc - 1) / 2;
    struct Image **images = malloc(num_images * sizeof(struct Image *));
    if (images == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    //Load all input images
    for (int i = 0; i < num_images; i++)
    {
        images[i] = load_image(argv[1 + i * 2]);
        if (images[i] == NULL)
        {
            fprintf(stderr, "Failed to load image %s.\n", argv[1 + i * 2]);
            for (int j = 0; j < i; j++)
            {
                free_image(images[j]);
            }
            free(images);
            return 1;
        }
    }

    // Process each image: apply blur, normalize, and save.
    for (int i = 0; i < num_images; i++)
    {
        struct Image *out_img = apply_BLUR(images[i]);
        if (out_img == NULL)
        {
            fprintf(stderr, "First process failed for image %s.\n", argv[1 + i * 2]);
            for (int j = 0; j < num_images; j++)
            {
                free_image(images[j]);
            }
            free(images);
            return 1;
        }

        if (!apply_NORM(out_img))
        {
            fprintf(stderr, "Second process failed for image %s.\n", argv[1 + i * 2]);
            free_image(out_img);
            for (int j = 0; j < num_images; j++)
            {
                free_image(images[j]);
            }
            free(images);
            return 1;
        }

        if (!save_image(out_img, argv[2 + i * 2]))
        {
            fprintf(stderr, "Saving image to %s failed.\n", argv[2 + i * 2]);
            free_image(out_img);
            for (int j = 0; j < num_images; j++)
            {
                free_image(images[j]);
            }
            free(images);
            return 1;
        }

        free_image(out_img);
    }

    for (int i = 0; i < num_images; i++)
    {
        free_image(images[i]);
    }

    free(images);
    return 0;
}