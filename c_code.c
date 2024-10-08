void SelectionSort(int n, int arr[n]) {
    int i, j, minIndex, n, temp ;
    for (i =  0 ; i <=  n - 2 ; ++i) {
        minIndex =  i;
        for (j =  i + 1 ; j <=  n - 1 ; ++j) {
            if ( arr[j] < arr[minIndex] ) {
                minIndex =  j;
            }
        }

        if ( minIndex != i ) {
            temp =  arr[i];
            arr[i] =  arr[minIndex];
            arr[minIndex] =  temp;
        }
    }
    return;
//translating endfunction or endprocedure
}


void InsertionSort(int n, int arr[n]) {
    int i, j, key ;
    for (i =  1 ; i <=  n - 1 ; ++i) {
        key =  arr[i];
        j =  i - 1;

        while ( j >= 0 && arr[j] > key ) {
            arr[j + 1] =  arr[j];
            j =  j - 1;
        }

        arr[j + 1] =  key;
    }
//translating endfunction or endprocedure
}

void MergeSort(int arr[], int l, int r) {
    int m ;
    if ( l < r ) {
        m =  (l + r) / 2;
        MergeSort(arr, l, m);
        MergeSort(arr, m + 1, r);
        Merge(arr, l, m, r);
    }
//translating endfunction or endprocedure
}

void Merge(int arr[], int l, int m, int r) {
    int n1, n2, i, j, k ;
    int L[m - l + 1], R[r - m] ;
    n1 =  m - l + 1;
    n2 =  r - m;

    for (i =  0 ; i <=  n1 - 1 ; ++i) {
        L[i] =  arr[l + i];
    }
    for (j =  0 ; j <=  n2 - 1 ; ++j) {
        R[j] =  arr[m + 1 + j];
    }

    i =  0;
    j =  0;
    k =  l;

    while ( i < n1 && j < n2 ) {
        if ( L[i] <= R[j] ) {
            arr[k] =  L[i];
            i =  i + 1;
        } else {
            arr[k] =  R[j];
            j =  j + 1;
        }
        k =  k + 1;
    }

    while ( i < n1 ) {
        arr[k] =  L[i];
        i =  i + 1;
        k =  k + 1;
    }

    while ( j < n2 ) {
        arr[k] =  R[j];
        j =  j + 1;
        k =  k + 1;
    }
//translating endfunction or endprocedure
}

int sum(int a , int b ) {
    return  a + b;
    //translating endfunction or endprocedure
}

int main() {
    int arr[] = {12, 11, 13, 5, 6, 7};
    int n, i ;
        n =  6;

    MergeSort(arr, 0, n - 1);

    for (i =  0 ; i <=  n - 1 ; ++i) {
        printf("%d", arr[i]);
    }
    return 0;
}
