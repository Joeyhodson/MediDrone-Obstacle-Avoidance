# Quick & dirty script -> easily converts ramPatch.hex from TMF8805 sensor into 2d C array

# note: manually remove last comma of 2d array
# ex intel hex code     ------>   ':101C1000FF8000D6EAF77C36807C00FF5D488E5D51'
# desired i2c sequence  ------>   'S41W084110FF8000D6EAF77C36807C00FF5D488E5D3BP'
# c array output        ------>   '{0x08,0x41,0x10,0xFF,0x80,0x00,0xD6,0xEA,0xF7,0x7C,0x36,0x80,0x7C,0x00,0xFF,0x5D,0x48,0x8E,0x5D,0x3B}'
#                               RAM Region ^   ^ #bytes                                                             must compute cs ^

# must manually fill in the other 3 arrays that are not of intel hex type: data_record.
# manually fill in the EOF encryption (last), second to last code, and initial base address code (first)

# use begin_every_line_new for actual i2c sequence dump as the extra 3 bytes are a waste of memory in mcu.

# intel hex code type lengths
data_record = 43

raw_ram_patch_file = "./main_app.hex"
c_style_2d_array_file = "./patch.txt"
c_vari_declaration_begin = "unsigned char patch[][] = {\n"
c_vari_declaration_end = "\n};"
begin_every_line = "{0x08,0x41,0x10,0x"
begin_every_line_new = "{0x"
begin_every_line_cs = "4110"
new_byte = ",0x"
newline = "},\n"

def fill(line, group, insert):
    line = str(line)
    return insert.join(line[i:i+group] for i in range(0, len(line), group))

def convert_to_c_style_array(line, checksum):
    line = line[9:-2]
    line = fill(line, 2, new_byte)
    line = begin_every_line_new + line
    return line + "," + hex(checksum) + newline

def get_sequence_in_bytes(sequence):
    byte_sequence = []
    for i in range(0, len(sequence), 2):
        byte_sequence.append(int(sequence[i:i+2], 16))
    return byte_sequence

def get_sum_all_but_last(sequence):
    sequence = sequence[9:]
    sequence = begin_every_line_cs + sequence
    print(sequence)
    bytes = get_sequence_in_bytes(sequence)
    sum = 0
    for byte in bytes[:-1]:
        sum += byte
    #print(hex(sum))
    print(hex(sum%256))
    return sum%256

def flip_bits(seq):
    return ''.join('1' if x == '0' else '0' for x in seq)

def get_checksum(val):
    if val == 256:
        return 0
    bin_string = "{0:08b}".format(val, 1)
    #print(bin_string)
    last_one = bin_string.rindex("1")
    bin_string_cp = bin_string[0:last_one]
    bin_string = flip_bits(bin_string_cp) + bin_string[last_one:]
    return int(bin_string, 2)

def get_checksum2(summed_sequence):
    # (Command + Data Size + Data) XOR 0xFF
    # (0x41 + 0x10 + 0x00 + 0x01 + 0x02 + ... + 0x03) XOR 0xFF
    return summed_sequence ^ 0xff

def main():
    out_file = open(c_style_2d_array_file, "a")
    out_file.write(c_vari_declaration_begin)
    with open(raw_ram_patch_file) as file:
        my_list = file.read().splitlines()
        for line in my_list:
            #line = line[1:-5]
            print(line)
            if len(line) == data_record:
                cs = get_checksum(get_sum_all_but_last(line) + 1)
                cs2 = get_checksum(get_sum_all_but_last(line))
                print(hex(cs))
                print(hex(cs2))
                if cs != cs2:
                    print("wrong checksum")
                    break
                print(convert_to_c_style_array(line, cs))
                out_file.write(convert_to_c_style_array(line, cs))
    out_file.write(c_vari_declaration_end)
main()