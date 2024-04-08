void test(int label) {
    switch (label) {
        case 1: {
            printf("reach case 1\n");
            break;
        }
        case 2: {
            printf("reach case 2\n");
            break;
        }
        case 3: {
            printf("reach case 3\n");
            break;
        }
        default: {
            printf("reach case default\n");
            break;
        }
    }
}

int main() {
    test(1);
    test(2);
    test(3);
    test(4);
    return 0;
}