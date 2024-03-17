#include "generate_pieces.h"
#include <opencv2/imgcodecs.hpp> // Include for cv::imread and cv::imwrite

void assignMemory(int height, int width, int X) {
    block = new Block[X];
    Block dull;
    for (int i = 0; i < X; i++) {
        block[i].image = new Pixel*[height];
        block[i].bins = new int[bin];
        block[i].original_idx = i;

        for (int j = 0; j < 4; j++)
            block[i].id[j] = false;
        for (int j = 0; j < bin; j++) {
            block[i].bins[j] = 0;
        }
        for (int j = 0; j < height; j++) {
            block[i].image[j] = new Pixel[width];
        }
    }
    dull.image = new Pixel*[height];
    dull.bins = new int[bin];
    for (int j = 0; j < height; j++)
        dull.image[j] = new Pixel[width];
}

void generateImages(cv::Mat img, int n, int height, int width) {
    int start, stop;
    int X = n * n;
    for (int i = 0; i < X; i++) {
        start = i / n;
        stop = i % n;
        start = start * height;
        stop = stop * width;

        for (int j = 0; j < height; j++) {
            for (int k = 0; k < width; k++) {
                cv::Vec3b pix = img.at<cv::Vec3b>(j + start, k + stop);
                for (int h = 0; h < 3; h++) {
                    block[i].image[j][k].val[h] = pix[h];
                }
            }
        }
    }
}

vector<Block> permute(int n) {
    vector<Block> temp;
    for (int j = 0; j < n; j++) {
        block[j].idx = j;
        temp.pb(block[j]);
    }
    for (int j = 0; j < n; j++) {
        int rnd = rand() % n;
        swap(temp[j], temp[rnd]);
    }
    return temp;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "No file name found. Please pass a file name as a parameter." << std::endl;
        exit(1);
    }

    cv::Mat img = cv::imread(argv[1]);
    if (img.empty()) {
        std::cerr << argv[1] << ": File not found." << std::endl;
        exit(1);
    }

    int height, width, n, len;
    std::cout << "Image dimensions: " << img.rows << " " << img.cols << std::endl;
    std::cout << "Enter side length of square piece: ";
    std::cin >> len;

    if (len < 1) {
        std::cerr << "Invalid side length." << std::endl;
        exit(1);
    }

    n = std::min(img.rows / len, img.cols / len);
    height = width = len;

    assignMemory(height, width, n * n);
    generateImages(img, n, height, width);

    vector<Block> permuted = permute(n * n);
    std::ofstream metadataFile("./generated_pieces/original_positions.txt");

    for (int i = 0; i < n * n; i++) {
        char filename[50] = "./generated_pieces/";
        char fileid[10];
        sprintf(fileid, "%d.jpg", i + 1);
        cv::Mat pieceImg = cv::Mat::zeros(height, width, CV_8UC3);

        for (int j = 0; j < height; j++) {
            for (int k = 0; k < width; k++) {
                // Access the pixel values from permuted[i].image[j][k]
                Pixel& pixel = permuted[i].image[j][k];
                // Assign individual color channel values to cv::Vec3b
                pieceImg.at<cv::Vec3b>(j, k) = cv::Vec3b(pixel.val[0], pixel.val[1], pixel.val[2]);
            }
        }

        std::string fullPath = std::string(filename) + std::string(fileid);
        cv::imwrite(fullPath, pieceImg);
        metadataFile << i + 1 << "," << permuted[i].original_idx << std::endl;
        printf("Generated %s\n", fileid);
    }

    metadataFile.close();
    std::cout << "Picture Broken into total " << n * n << " pieces." << std::endl;
    std::cout << "N = " << n << std::endl;

    return 0;
}
