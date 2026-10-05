int main() {
    int x = 0;
    while (x < 3) {
        x = x + 1;
        if (x == 2) {
            x = x + 1;
        }
    }

    int n = 2;
    while (n > 0) {
        int inner = 1;
        while (inner > 0) {
            inner = inner - 1;
        }
        n = n - 1;
    }

    return x;
}
