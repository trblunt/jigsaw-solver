#include "generate_pieces.h"
#include <opencv2/imgcodecs.hpp> // Include for cv::imread and cv::imwrite
#include <random>

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

vector<Block> permute(int n, int seed) {
    std::random_device rd;
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(0, n - 1);

    vector<Block> temp;
    for (int j = 0; j < n; j++) {
        block[j].idx = j;
        temp.pb(block[j]);
    }
    for (int j = 0; j < n; j++) {
        int rnd = distribution(generator);
        swap(temp[j], temp[rnd]);
    }
    return temp;
}

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 5) {
        std::cerr << "No file name found. Please pass a file name as a parameter." << std::endl;
        exit(1);
    }

    cv::Mat img = cv::imread(argv[1]);
    if (img.empty()) {
        std::cerr << argv[1] << ": File not found." << std::endl;
        exit(1);
    }

    int height, width, n, len;
    string dir = "./generated_pieces";
    int seed = time(0);
    std::cout << "Image dimensions: " << img.rows << " " << img.cols << std::endl;
    if (argc == 5) {
        len = std::stoi(argv[2]);
        dir = argv[3];
        seed = std::stoi(argv[4]);
        // If dir doesn't end with a '/', add it
        if (dir.back() != '/') {
            dir += '/';
        }
        std::cout << "Side length of square piece: " << len << std::endl;
    } else {
        std::cout << "Enter side length of square piece: ";
        std::cin >> len;
    }

    if (len < 1) {
        std::cerr << "Invalid side length." << std::endl;
        exit(1);
    }

    n = std::min(img.rows / len, img.cols / len);
    height = width = len;

    assignMemory(height, width, n * n);
    generateImages(img, n, height, width);

    vector<Block> permuted = permute(n * n, seed);
    std::ofstream metadataFile(dir + "original_positions.txt");

    for (int i = 0; i < n * n; i++) {

        cv::Mat pieceImg = cv::Mat::zeros(height, width, CV_8UC3);
        string fileid = std::to_string(i + 1) + ".jpg";
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < width; k++) {
                // Access the pixel values from permuted[i].image[j][k]
                Pixel& pixel = permuted[i].image[j][k];
                // Assign individual color channel values to cv::Vec3b
                pieceImg.at<cv::Vec3b>(j, k) = cv::Vec3b(pixel.val[0], pixel.val[1], pixel.val[2]);
            }
        }

        std::string fullPath = dir + fileid;
        cv::imwrite(fullPath, pieceImg);
        metadataFile << i + 1 << "," << permuted[i].original_idx << std::endl;
        printf("Generated %s\n", fileid.c_str());
    }

    metadataFile.close();
    std::cout << "Picture Broken into total " << n * n << " pieces." << std::endl;
    std::cout << "N = " << n << std::endl;

    return 0;
}
