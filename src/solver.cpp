#include <iostream>
#include <time.h>
#include <map>
#include "image.hpp"
#include "MST_solver.h"
#include "GA_solver.h"
#include <opencv2/imgcodecs.hpp> // For image I/O functions
#include <opencv2/core.hpp> // For Mat
#include <opencv2/imgproc.hpp> // For image processing functions

using namespace std;

#define TIME_LIMIT 15.0

int N, X;
Images pieces;

void saveResult(const vector<Block>& ans, int height, int width, const string& output) {
    cv::Mat finalImage = cv::Mat::zeros(height * N, width * N, CV_8UC3);
    for (int i = 0; i < X; i++) {
        int startRow = (i / N) * height;
        int startCol = (i % N) * width;
        for (int j = 0; j < height; j++) {
            for (int k = 0; k < width; k++) {
                cv::Vec3b& pixel = finalImage.at<cv::Vec3b>(startRow + j, startCol + k);
                for (int h = 0; h < 3; h++) {
                    pixel[h] = static_cast<unsigned char>(ans[i].image[j][k].val[h]);
                }
            }
        }
    }
    cv::imwrite(output, finalImage);
}

double calculateNCS(const vector<Block>& solved, int N) {
    int correctRelations = 0;
    int totalRelations = 0;

    for (int i = 0; i < solved.size(); ++i) {
        /*
        int originalX = solved[i].original_idx % N;
        int originalY = solved[i].original_idx / N;

        // Right neighbor check
        if (originalX < N - 1) {
            int rightNeighborOriginalIdx = solved[i].original_idx + 1;
            if (i % N < N - 1 && solved[i + 1].original_idx == rightNeighborOriginalIdx) {
                correctRelations++;
            } else if(i % N < N - 1) {
            }
            totalRelations++;
        }

        // Bottom neighbor check
        if (originalY < N - 1) {
            int bottomNeighborOriginalIdx = solved[i].original_idx + N;
            if (i + N < solved.size() && solved[i + N].original_idx == bottomNeighborOriginalIdx) {
                correctRelations++;
            } else if(i + N < solved.size()) {
            }
            totalRelations++;
        }
        */
        if (i == solved[i].original_idx) {
            correctRelations++;
        }
        totalRelations++;
    }

    double ncs = totalRelations > 0 ? static_cast<double>(correctRelations) / totalRelations : 0;
    return ncs;
}



int main(int argc, char* argv[]) {
    int given_N = -1;
    string dir = "./generated_pieces";
    if (argc == 3) {
        given_N = atoi(argv[1]);
        dir = argv[2];
        // If dir doesn't end with a slash, add one
        if (dir.back() != '/') {
            dir += '/';
        }
    } else if (argc != 1) {
        cout << "Usage: " << argv[0] << " [N]" << endl;
        return 1;
    }

    pieces.initializeAll(given_N, dir);
    N = pieces.N;
    X = N * N;

    vector<Block> scrambled = pieces.getScrambledImage();
    saveResult(scrambled, pieces.height, pieces.width, dir + "scrambled_image.jpg");

    vector<Block> ans;
    // if (N > 15) {
    //     MST mst(N, &pieces);
    //     ans = mst.get_mst(pieces.height, pieces.width);
    // } else {
        GA ga(N, &pieces);
        ans = ga.runAlgo(pieces.height, pieces.width);
    // }
    saveResult(ans, pieces.height, pieces.width, dir + "solved_image.jpg");

    cout << "NCS: " << calculateNCS(ans, N) << endl;

    return 0;
}
