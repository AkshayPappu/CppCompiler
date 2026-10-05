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

    bool large = result >= 10;

    if (large) {
        return result;
    }

    return 0;
}
