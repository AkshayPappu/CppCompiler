int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

int main() {
    int x = 5;
    int y = 10;

    int result = max(x, y);

    while (result < 20) {
        result = result + 1;
    }

    bool large = result >= 20;

    if (large && result != 25) {
        return result;
    }

    return 0;
}
