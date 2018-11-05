# calc_imaginary_ir
Пример генерации промежуточного представления кода калькулятора для воображаемой регистровой машины
```
>>> (1 + 2) / 3 * 5
```
```
%x1 = 1.000000
%x2 = 2.000000
%addtmp3 = add %x1 %x2
%x4 = 3.000000
%divtmp5 = div %addtmp3 %x4
%x6 = 5.000000
%multmp7 = mul %divtmp5 %x6
%result = %multmp7
```
https://ps-group.github.io/compilers/stack_and_register
