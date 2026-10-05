int main() {
    int x = 5;

    if (x > 0) {
        x = 1;
    }

    if (x < 0) {
        return 2;
    } else {
        x = 3;
    }

    if (x == 3) {
        return 4;
    }

    return 0;
}
