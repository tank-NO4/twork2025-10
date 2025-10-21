#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>
#include <omp.h>
//deepseek大人的内存测量
long get_memory_usage_kb() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss;
    }
    return 0;
}

long get_precise_memory_kb() {
    FILE *file = fopen("/proc/self/statm", "r");
    if (file == NULL) return 0;
    
    long size, resident, share, text, lib, data, dt;
    if (fscanf(file, "%ld %ld %ld %ld %ld %ld %ld", 
               &size, &resident, &share, &text, &lib, &data, &dt) != 7) {
        fclose(file);
        return 0;
    }
    fclose(file);
     long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
    if (page_size_kb <= 0) page_size_kb = 4;
    
    return resident * page_size_kb;
}

long measure_memory_increment(void (*sort_func)(double[], int, int), double arr[], int size) {
    double *test_arr = (double*)malloc(size * sizeof(double));
    if (!test_arr) return -1;
    
    memcpy(test_arr, arr, size * sizeof(double));
    
    long memory_before = get_precise_memory_kb();
    
    sort_func(test_arr, 0, size-1);
    
    long memory_after = get_precise_memory_kb();
    
    free(test_arr);
     long increment = memory_after - memory_before;
    return increment > 0 ? increment : 0;
}
//交换
void swap(double *a, double *b){
    double temp = *a;
    *a = *b;
    *b = temp;
}
//三分
void apart(double arr[], int low, int high, int *pivot1, int *pivot2){
    if (low >= high) {
        *pivot1 = low;
        *pivot2 = high;
        return;
    }
  int mid = low + (high - low) / 2;
    if (arr[low] > arr[high]) swap(&arr[low], &arr[high]);
    if (arr[low] > arr[mid]) swap(&arr[low], &arr[mid]);
    if (arr[mid] > arr[high]) swap(&arr[mid], &arr[high]);
    
    swap(&arr[low], &arr[mid]);
    
    double pivot1_val = arr[low];
    double pivot2_val = arr[high];
    
    int i = low + 1;
    int a = low + 1;
    int b = high - 1;
    
    while (i <= b){
        if(arr[i] < pivot1_val){
            swap(&arr[i], &arr[a]);
            a++;
            i++;
        } else if (arr[i] > pivot2_val){
            swap(&arr[i], &arr[b]);
            b--;
        } else {
            i++;
        }
    }
    
    swap(&arr[low], &arr[a-1]);
    swap(&arr[high], &arr[b+1]);
    
    *pivot1 = a - 1;
    *pivot2 = b + 1;
}
//插入排序
void intersort(double arr[], int low, int high){
    for (int i = low + 1; i <= high; i++){
        double key = arr[i];
        int c = i - 1;
        while (c >= low && arr[c] > key){
            arr[c+1] = arr[c];
            c--;
        }
        arr[c+1] = key;
    }
}
//栈的五个操作
typedef struct {
    int low;
    int high;
} stackitem;
//二元栈方便理解
typedef struct {
    stackitem *items;
    int top;
    int capacity;
} Stack;

Stack* createstack(int capacity){
    Stack *stack = (Stack*)malloc(sizeof(Stack));
    stack->items = (stackitem*)malloc(capacity * sizeof(stackitem));
    stack->top = -1;
    stack->capacity = capacity;
    return stack;
}

void push(Stack *stack, int low, int high){
    if(stack->top < stack->capacity-1){
        stack->top++;
        stack->items[stack->top].low = low;
        stack->items[stack->top].high = high;
    }
}

int pop(Stack *stack, int *low, int *high){
    if (stack->top >= 0){
        *low = stack->items[stack->top].low;
        *high = stack->items[stack->top].high;
        stack->top--;
        return 1;
    }
    return 0;
}

int empty(Stack *stack){
    return stack->top == -1;
}

void freestack(Stack *stack){
    free(stack->items);
    free(stack);
}
//递归快速排序
void quicksort1(double arr[], int low, int high){
    if (low < high){
        if (high - low < 10){
            intersort(arr, low, high);
            return;
        }
        int pivot1, pivot2;
        apart(arr, low, high, &pivot1, &pivot2);
        quicksort1(arr, low, pivot1-1);
        quicksort1(arr, pivot1, pivot2-1);
        quicksort1(arr, pivot2, high);
    }
}
//非递归快速排序
void quicksort2(double arr[], int low, int high) {
    if (high - low <= 0) return;
    
    int stack_low[100000], stack_high[100000];
    int top = -1;
    
    stack_low[++top] = low;
    stack_high[top] = high;
    
    while (top >= 0) {
        int l = stack_low[top];
        int h = stack_high[top];
        top--;
        
        if (l >= h) continue;
        
        if (h - l <= 20) {
            intersort(arr, l, h);
            continue;
        }
        
        int p1, p2;
        apart(arr, l, h, &p1, &p2);
        int left_size = (p1 - 1) - l;
        int right_size = h - (p2 + 1);
        
        if (left_size > right_size) {
            if (right_size > 0) {
                stack_low[++top] = p2 + 1;
                stack_high[top] = h;
            }
            if (left_size > 0) {
                stack_low[++top] = l;
                stack_high[top] = p1 - 1;
            }
        } else {
            if (left_size > 0) {
                stack_low[++top] = l;
                stack_high[top] = p1 - 1;
            }
            if (right_size > 0) {
                stack_low[++top] = p2 + 1;
                stack_high[top] = h;
            }
        }
        
        int mid_size = (p2 - 1) - (p1 + 1);
        if (mid_size > 0) {
            stack_low[++top] = p1 + 1;
            stack_high[top] = p2 - 1;
        }
    }
}
//合并
void merge(double arr[], int left, int mid, int right){
    int n1 = mid - left + 1;
    int n2 = right - mid;
    double *L = (double *)malloc(n1 * sizeof(double));
    double *R = (double *)malloc(n2 * sizeof(double));
    
    for(int i = 0; i < n1; i++){
        L[i] = arr[left + i];
    }
    for(int j = 0; j < n2; j++){
        R[j] = arr[mid + 1 + j];
    }
    
    int i = 0;
    int j = 0;
    int k = left;
    
    while (i < n1 && j < n2){
        if(L[i] < R[j]){
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    
    while(i < n1){
        arr[k] = L[i];
        i++;
        k++;
    }
    
    while(j < n2){
        arr[k] = R[j];
        j++;
        k++;
    }
    
    free(L);
    free(R);
}
//归并排序
void mergesort(double arr[], int left, int right){
    if(left < right){
        int mid = left + (right - left)/2;
        if(right - left > 1000){
            #pragma omp parallel
            {
                #pragma omp single nowait
                {
                    #pragma omp task
                    mergesort(arr, left, mid);
                    #pragma omp task
                    mergesort(arr, mid+1, right);
                    #pragma omp taskwait
                }   
            }
        } else {
            mergesort(arr, left, mid);
            mergesort(arr, mid+1, right);
        }
        merge(arr, left, mid, right);
    }
}
//随机数组
void generate_random_array(double arr[], int size, double min, double max) {
    for (int i = 0; i < size; i++) {
        arr[i] = ((double)rand() / RAND_MAX) * (max - min) + min;
    }
}
//打印
void printarr(double arr[], int size) {
    int print_size = (size > 20) ? 20 : size;
    for (int i = 0; i < print_size; i++) {
        printf("%.2f, ", arr[i]);
    }
    if (size > 20) {
        printf("... (总共 %d 个元素)", size);
    }
    printf("\n");
}
//检查是否正确
int check(double arr[], int size) {
    for (int i = 1; i < size; i++) {
        if (arr[i] < arr[i-1]) return 0;
    }
    return 1;
}
//复制
void copyarr(double dest[], double src[], int size) {
    memcpy(dest, src, size * sizeof(double));
}
//生成测试数组
void test_algorithm(const char* name, double arr[], int size, void (*sort_func)(double[], int, int)) {
    printf("[%s]\n", name);
    
    double *test_arr = (double*)malloc(size * sizeof(double));
    if (!test_arr) {
        printf("错误: 测试数组分配失败\n");
        return;
    }
    copyarr(test_arr, arr, size);
    
    clock_t start = clock();
    sort_func(test_arr, 0, size-1);
    clock_t end = clock();
    double time_used = (double)(end - start) / CLOCKS_PER_SEC;
    
    long memory_used = measure_memory_increment(sort_func, arr, size);
    
    printf("正确性: %s\n", check(test_arr, size) ? "✓" : "✗");
    printf("用时: %.6f秒\n", time_used);
    
    if (memory_used >= 0) {
        printf("内存使用: %ld KB\n", memory_used);
        if (strstr(name, "归并") != NULL) {
            long theoretical = (size * sizeof(double)) / 1024;
            printf("理论O(n)内存: %ld KB\n", theoretical);
        } else {
            printf("理论O(log n)内存: < 10 KB\n");
        }
    } else {
        printf("内存测量: 无法准确测量\n");
    }
    printf("\n");
    
    free(test_arr);
}
//deepseek大人的改进测试函数
void analyze_memory_patterns(double arr[], int size) {
    printf("=== 内存使用模式分析 ===\n");
    printf("数据规模: %d 元素 (%ld KB 数组大小)\n", size, (size * sizeof(double)) / 1024);
    
    int test_sizes[] = {1000, 5000, 10000, 50000, 100000};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        int test_size = test_sizes[i];
        if (test_size > size) continue;
        
        printf("\n测试规模: %d 元素\n", test_size);
        
        long mem1 = measure_memory_increment(quicksort1, arr, test_size);
        printf("  递归快速排序: %ld KB\n", mem1);
        
        long mem2 = measure_memory_increment(quicksort2, arr, test_size);
        printf("  非递归快速排序: %ld KB\n", mem2);
        
        long mem3 = measure_memory_increment(mergesort, arr, test_size);
        printf("  归并排序: %ld KB\n", mem3);
    }
    printf("\n");
}
//主函数
int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    int size = 10000;
    char *algorithm = "all";
    
    if (argc > 1) {
        size = atoi(argv[1]);
    }
    if (argc > 2) {
        algorithm = argv[2];
    }
    
    printf("=== 排序算法性能测试 (浮点数版本) ===\n");
    printf("数据规模: %d\n", size);
    printf("测试算法: %s\n\n", algorithm);
    
    double *origin = (double*)malloc(size * sizeof(double));
    if (!origin) {
        printf("错误: 内存分配失败！\n");
        return -1;
    }
    
    printf("生成的测试数据 (前20个元素): ");
    generate_random_array(origin, size, 1.0, size * 10.0);
    printarr(origin, size);
    printf("\n");
    
    if (strcmp(algorithm, "all") == 0) {
        analyze_memory_patterns(origin, size);
    }
    
    if (strcmp(algorithm, "all") == 0 || strcmp(algorithm, "quicksort1") == 0) {
        test_algorithm("递归快速排序", origin, size, quicksort1);
    }
    
    if (strcmp(algorithm, "all") == 0 || strcmp(algorithm, "quicksort2") == 0) {
        test_algorithm("非递归快速排序", origin, size, quicksort2);
    }
    
    if (strcmp(algorithm, "all") == 0 || strcmp(algorithm, "mergesort") == 0) {
        test_algorithm("归并排序", origin, size, mergesort);
    }
    
    free(origin);
    return 0;
}