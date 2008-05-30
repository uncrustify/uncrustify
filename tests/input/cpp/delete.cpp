
void x(int **d) {
delete *d;
}

void x(int& d) {
delete &d;
}


