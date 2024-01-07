#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>
#include <ctime>
#include <cassert>

class NQueens {
private:
    std::vector<std::vector<int>> board;
    std::vector<std::pair<int, int>> queenPositions;
    int n;

public:
    NQueens(int n) : n(n) {
        std::tie(board, queenPositions) = getNewBoard(n);
    }

    std::pair<std::vector<std::vector<int>>, std::vector<std::pair<int, int>>> getNewBoard(int n) {
        std::vector<std::vector<int>> newBoard(n, std::vector<int>(n, 0));
        std::vector<std::pair<int, int>> queensPos;
        
        srand(time(nullptr));

        for (int x = 0; x < n; ++x) {
            int randomIndex = rand() % n;
            newBoard[x][randomIndex] = 1;
            queensPos.push_back(std::make_pair(x, randomIndex));
        }

        return std::make_pair(newBoard, queensPos);
    }

    bool allQueensSafe() {
        for (auto pos : queenPositions) {
            if (UnderAttack(pos)) {
                return false;
            }
        }
        return true;
    }

    bool attackViaCol(std::pair<int, int> pos) {
        for (auto queen : queenPositions) {
            if (pos.second == queen.second && queen != pos) {
                return true;
            }
        }
        return false;
    }

    bool attackViaRow(std::pair<int, int> pos) {
        for (auto queen : queenPositions) {
            if (pos.first == queen.first && queen != pos) {
                return true;
            }
        }
        return false;
    }

    bool attackViaDiagonal(std::pair<int, int> pos) {
        for (auto queen : queenPositions) {
            if (abs(queen.first - pos.first) == abs(queen.second - pos.second) && queen != pos) {
                return true;
            }
        }
        return false;
    }

    bool UnderAttack(std::pair<int, int> position) {
        return attackViaDiagonal(position) || attackViaRow(position) || attackViaCol(position);
    }

    int specificQueenConflicts(std::pair<int, int> pos) {
        assert(std::find(queenPositions.begin(), queenPositions.end(), pos) != queenPositions.end());

        int count = 0;
        for (auto queen : queenPositions) {
            if (abs(queen.first - pos.first) == abs(queen.second - pos.second) && queen != pos) {
                count++;
            }

            if (pos.first == queen.first && queen != pos) {
                count++;
            }

            if (pos.second == queen.second && queen != pos) {
                count++;
            }
        }
        return count;
    }

    std::pair<int, int> pickRandomQueen() {
        int newIndex = rand() % n;
        return queenPositions[newIndex];
    }

    void printBoard() {
        for (auto queen : queenPositions) {
            std::cout << queen.first << " " << queen.second << std::endl;
        }
    }

    void moveQueen(std::pair<int, int> startPos, std::pair<int, int> endPos) {
        assert(board[startPos.first][startPos.second] == 1);
        
        board[startPos.first][startPos.second] = 0;
        board[endPos.first][endPos.second] = 1;
        
        queenPositions.erase(std::remove(queenPositions.begin(), queenPositions.end(), startPos), queenPositions.end());
        queenPositions.push_back(endPos);
    }

    std::vector<std::pair<int, int>> availablePositions(std::pair<int, int> pos) {
        std::vector<std::pair<int, int>> availablePos;
        for (int x = 0; x < n; ++x) {
            availablePos.push_back(std::make_pair(pos.first, x));
        }
        return availablePos;
    }
};
