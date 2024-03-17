#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <queue>
#include <utility>
#include <vector>
#include <fstream>
#include <cmath>
#include <unordered_map>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
#define pb push_back
#define bin 10
#define limit 1
#define ABS(Y) ((Y)<0?-(Y):(Y))
#define R 0
#define T 1
#define D 2
#define L 3
#define INF 1000000000
#define TIME_LIMIT 15.0

typedef std::pair<int,int> pii;
typedef std::pair<pii,int> ppi;

struct Pixel {
    float val[3]; // Assuming this is for color images; adjust as needed
};

struct Block {
    Pixel** image;
    bool id[4];
    int neigh[4];
    int* bins;
    int idx;
    int original_idx;
};

class Images {
public:
    vector<vector<double>> adjl, adjr, adjt, adjd;
    Block* block;
    Block dull;
    int height, width;
    int N, X;

    Images() : block(nullptr), N(0), X(0), height(0), width(0) {}

    void loadImages() {
        std::unordered_map<int, int> originalIndices; // Map scrambled index to original index
        std::ifstream metadataFile("generated_pieces/original_positions.txt");
        std::string line;
        while (std::getline(metadataFile, line)) {
            std::istringstream iss(line);
            int scrambledIndex, originalIndex;
            char separator; // To consume the comma separator
            if (iss >> scrambledIndex >> separator && separator == ',' && iss >> originalIndex) {
                originalIndices[scrambledIndex] = originalIndex;
            }
        }
        metadataFile.close();

        for (int i = 0; i < X; i++) {
            std::string filename = "generated_pieces/" + std::to_string(i + 1) + ".jpg";
            cv::Mat img = cv::imread(filename);
            if (img.empty()) {
                std::cerr << "Error loading: " << filename << std::endl;
                continue;
            }

            // Retrieve the original index using the scrambled index (i + 1)
            if (originalIndices.find(i + 1) != originalIndices.end()) {
                block[i].original_idx = originalIndices[i + 1];
            } else {
                std::cerr << "Original index for piece " << i + 1 << " not found in metadata." << std::endl;
                continue;
            }

            block[i].idx = i; // Assuming 'idx' needs to be the index in the scrambled sequence
            for (int j = 0; j < height; j++) {
                for (int k = 0; k < width; k++) {
                    cv::Vec3b pix = img.at<cv::Vec3b>(j, k);
                    for (int h = 0; h < 3; h++) {
                        block[i].image[j][k].val[h] = pix[h];
                    }
                }
            }
        }
    }


    vector<Block> getScrambledImage() {
        vector<Block> v;
        for (int i = 0; i < X; i++) v.pb(block[i]);
        return v;
    }

    void initializeAll(int givenN) {
        std::string firstImageFilename = "generated_pieces/1.jpg";
        cv::Mat firstImg = cv::imread(firstImageFilename, cv::IMREAD_COLOR);
        if (firstImg.empty()) {
            cerr << "Failed to load image: " << firstImageFilename << endl;
            return;
        }

        if (givenN > 0) {
            N = givenN;
        } else {
            cout << "Enter The Value of N in NxN : \n";
            cin >> N;
        }
        height = firstImg.rows;
        width = firstImg.cols;
        X = N * N;
        initializeVector(X);
        assignMemory();
        loadImages();
        insertInTopMatrix();
        insertInLeftMatrix();
    }

    void initializeVector(int n) {
        adjl.resize(n, vector<double>(n));
        adjr.resize(n, vector<double>(n));
        adjt.resize(n, vector<double>(n));
        adjd.resize(n, vector<double>(n));
    }

    double getWeight(vector<Block>& c, int k, Block b) {
        double ans = 0;
        int a = k / N, bb = k % N;

        // Array of direction offsets directly defined within the function
        // Up, Right, Down, Left
        int dx[4] = {0, 1, 0, -1};
        int dy[4] = {1, 0, -1, 0};

        for (int i = 0; i < 4; i++) {
            int a1 = a + dx[i];
            int b1 = bb + dy[i];

            if (a1 < 0 || a1 >= N) continue;
            if (b1 < 0 || b1 >= N) continue;

            int adjIndex = a1 * N + b1;
            if (c[adjIndex].idx == -1) continue;

            // Use a switch case or if-else to apply the correct adjacency based on direction
            switch (i) {
                case 0: ans += adjr[b.idx][c[adjIndex].idx]; break; // Right
                case 1: ans += adjd[b.idx][c[adjIndex].idx]; break; // Down
                case 2: ans += adjl[b.idx][c[adjIndex].idx]; break; // Left
                case 3: ans += adjt[b.idx][c[adjIndex].idx]; break; // Up
            }
        }
        return ans;
    }


    void assignMemory() {
        block = new Block[X];
        for (int i = 0; i < X; i++) {
            block[i].image = new Pixel*[height];
            for (int j = 0; j < height; j++) {
                block[i].image[j] = new Pixel[width];
            }
            block[i].bins = new int[bin];
            std::fill_n(block[i].bins, bin, 0);
            std::fill_n(block[i].id, 4, false);
        }
        dull.image = new Pixel*[height];
        for (int j = 0; j < height; j++) {
            dull.image[j] = new Pixel[width];
        }
        dull.bins = new int[bin];
        std::fill_n(dull.bins, bin, 0);
    }


    double SSD_left(const Block& sure, const Block& trial) {
        double ssd = 0.0;
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < limit; ++j) {
                for (int h = 0; h < 3; ++h) {
                    ssd += std::pow(sure.image[i][j].val[h] - trial.image[i][width - 1 - j].val[h], 2);
                }
            }
        }
        return ssd;
    }

    double SSD_top(const Block& sure, const Block& trial) {
        double ssd = 0.0;
        for (int i = 0; i < limit; ++i) {
            for (int j = 0; j < width; ++j) {
                for (int h = 0; h < 3; ++h) {
                    ssd += std::pow(sure.image[i][j].val[h] - trial.image[height - 1 - i][j].val[h], 2);
                }
            }
        }
        return ssd;
    }

    void insertInLeftMatrix() {
        for (int i = 0; i < X; ++i) {
            for (int j = 0; j < X; ++j) {
                if (i != j) {
                    double ssd = SSD_left(block[i], block[j]);
                    adjl[i][j] = ssd;
                    adjr[j][i] = ssd;
                }
            }
        }
    }

    void insertInTopMatrix() {
        for (int i = 0; i < X; ++i) {
            for (int j = 0; j < X; ++j) {
                if (i != j) {
                    double ssd = SSD_top(block[i], block[j]);
                    adjt[i][j] = ssd;
                    adjd[j][i] = ssd;
                }
            }
        }
    }

    // Destructor to clean up allocated memory
    ~Images() {
        for (int i = 0; i < X; ++i) {
            for (int j = 0; j < height; ++j) {
                delete[] block[i].image[j];
            }
            delete[] block[i].image;
            delete[] block[i].bins;
        }
        delete[] block;
    }

};
#endif