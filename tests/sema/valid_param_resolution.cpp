int compute(int a, int b, bool flag) {
    int result = a + b;
    if (flag) {
        result = result * 2;
    }
    return result;
}

int main() {
    return compute(3, 4, true);
}
