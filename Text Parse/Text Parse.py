from time import sleep
last_line = "0" 
def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


input_text = open("input.txt", "r")
num_lines = sum(1 for line in open('input.txt'))

file_name = "Output/" + (input("Name output file, default 'output': ")) + ".txt"
user_input = input("Clear output text file: ")

if file_name == 'Output/.txt':
    file_name =  "Output/output.txt"

if ( user_input == "y"):
    open(file_name, 'w').close()
    output_text = open(file_name, "w")
else:
    output_text = open(file_name, "w")

first_line_set = input("\nSet a first line, y for custom, d for default: ")
if first_line_set == "y":
    output_text.write(input("Set first line: ")+ "\n")
elif first_line_set == "d":
    output_text.write("Latitude, Longitude \n")
    
individual = input("Only show individual inputs: ")
altitude = input("Altitude, y to add on, s for seperate: ")
display = input("\nShow parsed output: " )
print("Parsing " + str(num_lines) + " lines of text \n")

for i in range(num_lines):   
    current_line = input_text.readline()
    if (i > 34):
        
        if (is_number(current_line[0]) and (altitude == "n") ):
            current_line = str(current_line[0:19])
            if individual == "y" and str(last_line) != current_line:
                last_line = current_line
                output_text.write(current_line + "\n")
                if display == "y":
                    print(current_line)
            elif individual == "n":
                output_text.write(current_line + "\n")
                if display == "y":
                    print(current_line)
                    
        if (is_number(current_line[0])) and altitude == "y":
            current_line = current_line[0:22]
            if individual == "y" and str(last_line) != current_line:
                last_line = current_line
                output_text.write(current_line + "\n")
                if display == "y":
                    print(current_line)
            elif individual == "n":
                output_text.write(current_line + "\n")
                if display == "y":
                    print(current_line)
                
        if (is_number(current_line[0])) and altitude == "s":
            current_line = current_line[20:22]
            if individual == "y" and str(last_line) != current_line:
                last_line = current_line
                output_text.write(current_line + "\n")
                if display == "y":
                    print(current_line)
            elif individual == "n":
                output_text.write(current_line + "\n")
                if display == "y":
                    print(current_line)
                
input_text.close()
output_text.close()