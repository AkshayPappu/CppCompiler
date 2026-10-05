int add(int a, int b) {
    return a + b;
}

bool isPositive(int n) {
    return n > 0;
}

void doNothing() {
    return;
}

int main() {
    int sum = add(1, 2);
    bool pos = isPositive(sum);
    doNothing();
    return sum;
}
