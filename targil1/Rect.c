#include <stdio.h>
#include <stdlib.h>

void __stdcall func(int base, int height, char* buffer);

int main()
{
	int base, height, size;
	char* buffer;

	printf("Enter base: ");
	scanf_s("%d", &base);

	printf("Enter height: ");
	scanf_s("%d", &height);

	size = (base + 1) * height + 1;
	buffer = malloc(size * sizeof(*buffer));

	if (buffer == NULL) {
		printf("memory allocation failed\n");
		return 1;
	}

	func(base, height, buffer);

	printf("%s", buffer);
	free(buffer);

	return 0;
}

void __stdcall func(int base, int height, char* buffer)
{
	int i = 0;
	int row = height;
	int col;

	while (row > 0) {
		col = base;

		while (col > 0) {
			if (row == height || row == 1 || col == base || col == 1) {
				buffer[i] = '*';
			}
			else {
				buffer[i] = ' ';
			}

			i++;
			col--;
		}

		buffer[i] = '\n';
		i++;

		row--;
	}

	buffer[i] = '\0';
}

