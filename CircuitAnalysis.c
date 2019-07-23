#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

struct file 
{
	int node_arr[20][2];
	float res[20];
	float source_voltage;
	float source_current;
	float source_res;
	int load_node;
	int load_res;
	int num_elements;
};

struct matrix
{
	float A, B, C, D;
};

struct output_data
{
	float Vin[2];
	float Iin[2];
	float Pin[2];
	float Zin[2];
	float Vout[2];
	float Iout[2];
	float Pout[2];
	float Zout[2];
	float Av[2];
	float Ai[2];
};

char *open_file();
char *delim_func(const char* data, const char* start_delim, const char* end_delim);
struct file assign_data(char* data_str);
struct matrix matrix_multiply(struct matrix matrix1, struct matrix matrix2);
struct matrix circuit_matrix(struct file data_struct);
struct output_data output_struct(struct file data_struct, struct matrix circuitmatrix);
void file_print(char* data_string, struct output_data outputdata);
char* comment_remove(const char* data, const char* start_delim, const char* end_delim);
char* strremove(char* str, const char* sub);

int main()
{
	//OPENS ADBC_input.dat and stores data string in data_string
	char* data_string = open_file();
	printf("The following Data was inputted:\n %s\n", data_string);

	//Returns data_struct containing sorted data for calculations
	struct file data_struct = assign_data(data_string);

	//Returns matrix struct containing overall circuit matrix
	struct matrix circuitmatrix = circuit_matrix(data_struct);
	printf("Overall Circuit Matrix A, B, C, D:\n %f\n %f\n %f\n %f\n", circuitmatrix.A, circuitmatrix.B, circuitmatrix.C, circuitmatrix.D);

	//Returns struct outputdata containing output values
	struct output_data outputdata = output_struct(data_struct, circuitmatrix);

	//Formats output file and output data
	file_print(data_string, outputdata);

	return(0);
}

char* open_file()
{
	//Opens file "ABCD_input.dat" and stores string in data_str
	FILE* input_data = fopen("ABCD_input.dat", "r");

	if (input_data == NULL) {
		return "No file opened";
	}

	//Attains length of input_data file stream
	fseek(input_data, 0, SEEK_END);
	long length = ftell(input_data);

	//Reset stream pointer and allocate memory for data string
	fseek(input_data, 0, SEEK_SET);
	char* data_str = malloc((length + 1) * sizeof(char));

	if (data_str == NULL) {
		return "Data not copied to string";
	}

	//Places file stream into block of memory
	long offset = 0;
	while (!feof(input_data) && offset < length) {
		offset += fread(data_str + offset, sizeof(char), length - offset, input_data);
	}
	
	//Null terminates data string
	data_str[offset] = '\0';

	//Loop to remove comments from data_str
	char* sub;

	//When there are comments in data_str
	while ((strstr(data_str, "#")) != NULL) {

		//Find location of the comment
		sub = comment_remove(data_str, "#", "\n");

		//Comment removed from data_str
		data_str = strremove(data_str, sub);
		sub = NULL;
	}
	
	//Null terminates string
	fclose(input_data);

	return data_str;
}

char* strremove(char* str, const char* sub)
{
	//Removes string sub from memory pointed to by str
	char* p, * q, * r;
	//Loops for sub within str
	if ((q = r = strstr(str, sub)) != NULL) {
		size_t len = strlen(sub);

		//Searches for sub string beyond pointer R + length
		while ((r = strstr(p = r + len, sub)) != NULL) {

			//Increments pointers where r contains pointer to SUB
			while (p < r)
				* q++ = *p++;
		}
		//Checknig for end of string str, if not end increment pointers
		while ((*q++ = *p++) != '\0')
			continue;
	}
	return str;
}


char* comment_remove(const char* data, const char* start_delim, const char* end_delim)
{
	//Returns string pointer of string between "start_delim" and "end_delim"
	char* target = NULL;
	char* start, * end;

	//Finds pointer to start of start_delim and end_delim
	start = strstr(data, start_delim);
	end = strstr(start, end_delim);

	//Adds 1 for length of end_delim (\n) 
	end += +1;

	//Allocates mnemory for string assignment and copies string between delmiters to target
	target = (char*)malloc(end - start + 1);
	memcpy(target, start, end - start);

	if (start == NULL) {
		return("empty");
	}

	//Null termination
	target[end - start] = '\0';

	return target;
}

struct file assign_data(char* data_str)
{
	struct file data_struct = {
	.node_arr = {0},
	.res = {0},
	.source_voltage = {0},
	.source_current = {0},
	.source_res = {0},
	.load_node = {0},
	.load_res = {0},
	.num_elements = {0}
	};

	//Assigns <CIRCUIT> node associations to data_struct.node_arr[][], and resistance value to data_struct.res[]
	char var;
	int i = 0;
	float  inputval = { 0 };

	//Attains string between "<CIRCUIT>" and "</CIRCUIT>"
	char* circuit_string = delim_func(data_str, "<CIRCUIT>", "</CIRCUIT>");

	//Tokenises string into lines for sscanf
	char* input_nodes = strtok(circuit_string, "\n");
	while (input_nodes != NULL)
	{
		//Circuit component connections stored in data_struct, where conductance is converted to resistance if required
		sscanf(input_nodes, "n1=%d n2=%d %c=%f\n", &data_struct.node_arr[i][0], &data_struct.node_arr[i][1], &var, &inputval);
		if (var == 'G') {
			data_struct.res[i] = 1 / inputval;
		}
		else {
			data_struct.res[i] = inputval;
		}

		//Resets input_node pointer
		input_nodes = strtok(NULL, "\n");
		i++;
	}

	//<TERMS> input for source V: data_struct.source_voltage, or I: data_struct.source_current for Norton, and associated R: data_struct.source_res
	char thv_type[5], thv_input[5];
	float source_val = { 0 }, source_input = { 0 };


	//Assigns <TERMS> block values to variables
	char* thv_source = delim_func(data_str, "<TERMS>", "</TERMS>");
	sscanf(thv_source, "%2s=%f %2s=%f\n", &thv_type, &source_val, &thv_input, &source_input);

	//Assigns source values for current or voltage
	if (thv_type[0] == 'V') {
		data_struct.source_voltage = source_val;

		if (thv_input[0] == 'R') {
			data_struct.source_res = source_input;
		}
		else {
			data_struct.source_res = 1 / source_input;
		}
	}
	else {
		data_struct.source_current = source_val;

		if (thv_input[0] == 'R') {
			data_struct.source_res = source_input;
		}
		else {
			data_struct.source_res = 1 / source_input;
		}
	}

	//<TERMS> input for load node connection and R value
	char* thv_sourcel2 = strstr(thv_source, "\n") + 1;
	sscanf(thv_sourcel2, "%d RL=%d\n", &data_struct.load_node, &data_struct.load_res);

	//Number of elements in a circuit
	int num_element = 0;
	while (data_struct.node_arr[num_element][0] != 0)
	{
		num_element++;
	}

	data_struct.num_elements = num_element;

	//Loop to ensure smaller node connection value is in 1st column of the array
	for (int c = 0; c < num_element + 1; c++)
	{
		if (data_struct.node_arr[c][1] < data_struct.node_arr[c][0] && data_struct.node_arr[c][1] != 0)
		{
			//Where a non-zero value is present, the smaller input node value is stored in the 1st line
			int swap7 = data_struct.node_arr[c][0];
			data_struct.node_arr[c][0] = data_struct.node_arr[c][1];
			data_struct.node_arr[c][1] = swap7;
			swap7 = 0;
		}
	}

	//Bubble sort by first node value
	for (int a = 0; a < num_element - 1; a++) //Loop for num_element number of elements
	{
		for (int b = 0; b < (num_element - a - 1); b++) //Last element which has been sorted already
		{
			if (data_struct.node_arr[b][0] > data_struct.node_arr[b + 1][0])
			{
				//Where there is a greater 1st node, the larger value's associated node values
				//and resistance is along the array
				int swap1 = data_struct.node_arr[b][0];
				data_struct.node_arr[b][0] = data_struct.node_arr[b + 1][0];
				data_struct.node_arr[b + 1][0] = swap1;
				swap1 = 0;

				int swap2 = data_struct.node_arr[b][1];
				data_struct.node_arr[b][1] = data_struct.node_arr[b + 1][1];
				data_struct.node_arr[b + 1][1] = swap2;
				swap2 = 0;

				float swap3 = data_struct.res[b];
				data_struct.res[b] = data_struct.res[b + 1];
				data_struct.res[b + 1] = swap3;
				swap3 = 0;
			}
		}
	}

	//Ensures shunts are ahead of resistors in the data array
	for (int c = 0; c < num_element + 1; c++)
	{
		if (data_struct.node_arr[c][0] == data_struct.node_arr[c + 1][0] && data_struct.node_arr[c][1] > data_struct.node_arr[c + 1][1])
		{
			//For a node i, if a shunt is present  before a series resistance it must be before it in the array
			int swap4 = data_struct.node_arr[c][0];
			data_struct.node_arr[c][0] = data_struct.node_arr[c + 1][0];
			data_struct.node_arr[c + 1][0] = swap4;
			swap4 = 0;

			int swap5 = data_struct.node_arr[c][1];
			data_struct.node_arr[c][1] = data_struct.node_arr[c + 1][1];
			data_struct.node_arr[c + 1][1] = swap5;
			swap5 = 0;

			float swap6 = data_struct.res[c];
			data_struct.res[c] = data_struct.res[c + 1];
			data_struct.res[c + 1] = swap6;
			swap6 = 0;
		}
	}

	return data_struct;
}

char* delim_func(const char *data, const char *start_delim, const char *end_delim)
{
	//Returns string pointer of string between "start_delim" and "end_delim"
	char *target = NULL;
	char *start, *end;

	//Finds pointer to end of start_delim and end_delim
	start = strstr(data, start_delim);
	start += strlen(start_delim);
	end = strstr(start, end_delim);

	//Allocates mnemory for string assignment and copies string between delmiters
	target = (char*)malloc(end - start + 1);
	memcpy(target, start, end - start);

	if (target == NULL) {
		printf("Error creating data string");
	}
	//Null termination and removal of \n character
	target[end - start] = '\0';
	target += 1;

	return target;
}

struct matrix matrix_multiply(struct matrix matrix1, struct matrix matrix2)
{
	//Multiplies matrix1 by matrix2
	struct matrix matrix = {
	.A = {0},
	.B = {0},
	.C = {0},
	.D = {0},
	};

	matrix.A = matrix1.A * matrix2.A + matrix1.B * matrix2.C;
	matrix.B = matrix1.A * matrix2.B + matrix1.B * matrix2.D;
	matrix.C = matrix1.C * matrix2.A + matrix1.D * matrix2.C;
	matrix.D = matrix1.C * matrix2.B + matrix1.D * matrix2.D;

	return matrix;
}

struct matrix circuit_matrix(struct file data_struct)
{
	//Calculates overall circuit matrix 

	//Initial identity matrix
	struct matrix matrix1 = { .A = {1},.B = {0},.C = {0},.D = {1} };
	struct matrix matrix2 = { .A = {0},.B = {0},.C = {0},.D = {0} };

	//Loop through every component j in circuit
	for (int j = 1; j < data_struct.num_elements + 1; j++)
	{
		//ABCD matrix for a shunt resistance
		if (data_struct.node_arr[j - 1][1] == 0) {
			matrix2.A = 1;
			matrix2.B = 0;
			matrix2.C = 1 / data_struct.res[j - 1];
			matrix2.D = 1;
		}
		//ABCD matrix for a series resistance
		else
		{
			matrix2.A = 1;
			matrix2.B = data_struct.res[j - 1];
			matrix2.C = 0;
			matrix2.D = 1;
		}
		//Matrix 1 contains the overall matrix of i components, therefore matrix 2 contains nxt component values
		matrix1 = matrix_multiply(matrix1, matrix2);
	}

	return matrix1;
}

struct output_data output_struct(struct file data_struct, struct matrix circuitmatrix)
{
	//Forming output struct
	struct output_data outputdata = {
	.Vin = {0},
	.Iin = {0},
	.Pin = {0},
	.Zin = {0},
	.Vout = {0},
	.Iout = {0},
	.Pout = {0},
	.Zout = {0},
	.Av = {0},
	.Ai = {0}
	};

	//Stores Zin in outputdata[0].Zin struct, and db equivalent in outputdata.Zin[1]
	outputdata.Zin[0] = (circuitmatrix.A * data_struct.load_res + circuitmatrix.B) / (circuitmatrix.C * data_struct.load_res + circuitmatrix.D);
	outputdata.Zin[1] = 20 * log10f(outputdata.Zin[0]);

	//Repeated for all possible output values
	outputdata.Zout[0] = (circuitmatrix.D * data_struct.source_res + circuitmatrix.B) / (circuitmatrix.C * data_struct.source_res + circuitmatrix.A);
	outputdata.Zout[1] = 20 * log10f(outputdata.Zout[0]);

	outputdata.Av[0] = 1 / (circuitmatrix.A + (circuitmatrix.B / data_struct.load_res));
	outputdata.Av[1] = 20 * log10f(outputdata.Av[0]);

	outputdata.Ai[0] = 1 / (circuitmatrix.C * data_struct.load_res + circuitmatrix.D);
	outputdata.Ai[1] = 20 * log10f(outputdata.Ai[0]);

	outputdata.Vin[0] = (data_struct.source_voltage * outputdata.Zin[0]) / (data_struct.source_res + outputdata.Zin[0]);
	outputdata.Vin[1] = 20 * log10f(outputdata.Vin[0]);

	outputdata.Iin[0] = data_struct.source_voltage / (data_struct.source_res + outputdata.Zin[0]);
	outputdata.Iin[1] = 20 * log10f(outputdata.Iin[0]);

	outputdata.Pin[0] = outputdata.Vin[0] * outputdata.Iin[0];
	outputdata.Pin[1] = 10 * log10f(outputdata.Pin[0]);

	outputdata.Vout[0] = outputdata.Vin[0] * outputdata.Av[0];
	outputdata.Vout[1] = 20 * log10f(outputdata.Vout[0]);

	outputdata.Iout[0] = outputdata.Iin[0] * outputdata.Ai[0];
	outputdata.Iout[1] = 20 * log10f(outputdata.Iout[0]);

	outputdata.Pout[0] = outputdata.Vout[0] * outputdata.Iout[0];
	outputdata.Pout[1] = 10 * log10f(outputdata.Pout[0]);

	return outputdata;
}

void file_print(char* data_string, struct output_data outputdata)
{
	//Formatting output data and creating file
	
	char outval[13][30];
	int i = 0;

	//Returns string between deliminations "<OUTPUT>" and "</OUTPUT>"
	char* output_string = delim_func(data_string, "<OUTPUT>", "</OUTPUT>");
	char* output_str = strtok(output_string, "\n=");

	//Tokens are used to input data from string into outval[j] by line or '=' delimination
	while (output_str != NULL)
	{
		sscanf(output_str, "%[^\n]s", &outval[i - 1][30]);
		output_str = strtok(NULL, "\n");
		i++;
	}

	//Initializing array variables
	char print_type[200] = "";
	char print_unit[200] = "";
	char print_val[200] = "";
	char fstr[12][15];

	//Lines from outval[2] contain an data output format
	for (int j = 2; j < 12; j++)
	{
		//Each output variable type is concatenated to form the output type string (print_type)
		if ((strstr(outval[j], "Vin")) != NULL) {
			strcat(print_type, "Vin, ");

			//If the variable is to be outputted as dB, it is concatenated to the print_unit string
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBV, ");

				//Converts float to string type for concatenation
				sprintf(fstr[j], "%13.4e", outputdata.Vin[1]);
				
				//When output is in dB, the float for the dB variable type is concatenated to print_val
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "V, ");
				sprintf(fstr[j], "%13.4e", outputdata.Vin[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
		}

		if ((strstr(outval[j], "Iin")) != NULL) {
			strcat(print_type, "Iin, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBI, ");
				sprintf(fstr[j], "%13.4e", outputdata.Iin[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "I, ");
				sprintf(fstr[j], "%13.4e", outputdata.Iin[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Pin")) != NULL) {
			strcat(print_type, "Pin, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBW, ");
				sprintf(fstr[j], "%13.4e", outputdata.Pin[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "W, ");
				sprintf(fstr[j], "%13.4e", outputdata.Pin[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Zin")) != NULL) {
			strcat(print_type, "Zin, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBOhm, ");
				sprintf(fstr[j], "%13.4e", outputdata.Zin[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "Ohm, ");
				sprintf(fstr[j], "%13.4e", outputdata.Zin[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Vout")) != NULL) {
			strcat(print_type, "Vout, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBV, ");
				sprintf(fstr[j], "%13.4e", outputdata.Vout[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "V, ");
				sprintf(fstr[j], "%13.4e", outputdata.Vout[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Iout")) != NULL) {
			strcat(print_type, "Iout, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBI, ");
				sprintf(fstr[j], "%13.4e", outputdata.Iout[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "I, ");
				sprintf(fstr[j], "%13.4e", outputdata.Iout[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Pout")) != NULL) {
			strcat(print_type, "Pout, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBW, ");
				sprintf(fstr[j], "%13.4e", outputdata.Pout[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "W, ");
				sprintf(fstr[j], "%13.4e", outputdata.Pout[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Zout")) != NULL) {
			strcat(print_type, "Zout, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dBOhm, ");
				sprintf(fstr[j], "%13.4e", outputdata.Zout[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "Ohm, ");
				sprintf(fstr[j], "%13.4e", outputdata.Zout[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Av")) != NULL) {
			strcat(print_type, "Av, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dB, ");
				sprintf(fstr[j], "%13.4e", outputdata.Av[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "L, ");
				sprintf(fstr[j], "%13.4e", outputdata.Av[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}

		}

		if ((strstr(outval[j], "Ai")) != NULL) {
			strcat(print_type, "Ai, ");
			if ((strstr(outval[j], "dB")) != NULL)
			{
				strcat(print_unit, "dB, ");
				sprintf(fstr[j], "%13.4e", outputdata.Ai[1]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
			else
			{
				strcat(print_unit, "L, ");
				sprintf(fstr[j], "%13.4e", outputdata.Ai[0]);
				if (j < 11) {
					strcat(fstr[j], ", ");
					strcat(print_val, fstr[j]);
				}
				else {
					strcat(print_val, fstr[j]);
				}
			}
		}
	}

	//Output file stream is created and named from outval[1]
	char filename[30];
	sprintf(filename, "%s", outval[1]);
	FILE* output_file = fopen(filename, "w+");

	//Each print_xxx type string is outputted to the file
	fprintf(output_file, "%s\n", print_type);
	fprintf(output_file, "%s\n", print_unit);
	fprintf(output_file, "%s\n", print_val);

	return(0);
}