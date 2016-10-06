Архиватор, использующий арифметический либо ppm2 алгоритмы сжатия текстовых данных.
Компиляция:
gcc -Wall -g archive_ari_ppm2.c -o archivator
Использование:
./archivator [c|d] input_text_file output_file [ari|ppm2]
c - заархивировать(compress)
d - разархивировать(decompress)
ari - арифметическое сжатие
ppm2 - Prediction by Partial Matching — предсказание по частичному совпадению метод сжатия.
(ppm2 может получить ошибку сегментирования из-за нехватки памяти)
