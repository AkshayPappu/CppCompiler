int side() {
    return 1;
}

int main() {
    bool a = false && side() == 1;
    bool b = true || side() == 1;
    bool c = true && false || true;
    if (a || b && c) {
        return 1;
    }
    return 0;
}
