//y[n] = sum(bk*x) - sum(ak*y)

float sumB = 0;
float sumA = 0;
for (int k = 0; k<=N; k++){
    sumB += B[k]*x[k];
}
for(int k=1; k<=N; k++){
    sumA += A[k]*y[k];
}

float currentY = SumB-SumA;
float currentX = 1;

for(int k=N; k>=0; k--){
    y[k-1] = y[k];
    x[k-1] = x[k];
}

y[0] = currentY;
x[0] = currentX;