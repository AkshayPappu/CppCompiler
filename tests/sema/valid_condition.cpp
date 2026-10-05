int main() {
    int x = 5;
    bool flag = true;

    if (flag) {
        x = 10;
    }

    if (x > 5) {
        x = 20;
    } else {
        x = 30;
    }

    while (x < 100) {
        x = x + 1;
    }

    while (flag) {
        flag = false;
    }

    return x;
}
