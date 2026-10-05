int add(int a, int b) {
    return a + b;
}

void noop() {
    return;
}

int main() {
    noop();
    return add(3, 4);
}

int later() {
    return 1;
}
