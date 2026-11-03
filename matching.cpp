#include "matching.h"

int calculateScore(LostItem lost, Report found) {
    int score = 0;

    if(lost.category == found.category) {
        score += 4;
    }

    if(lost.itemName == found.itemName) {
        score += 3;
    }

    if(lost.color == found.color) {
        score += 2;
    }

    if(lost.location == found.location) {
        score += 2;
    }

    if(found.dateFound >= lost.dateLost && found.dateFound <= lost.dateLost + 7) {
        score += 1;
    }

    return score;
}
