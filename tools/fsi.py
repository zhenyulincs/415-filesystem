# free space inspector
#!/usr/bin/env python3
import sys

def hex_to_bits(hex_string):
    # Convert hex string to binary string
    binary_string = bin(int(hex_string, 16))[2:].zfill(8)
    # Reverse the binary string
    reversed_string = binary_string[::-1]
    # Return the reversed binary string
    return reversed_string

# Get the hex bytes from command line arguments
hex_bytes = sys.argv[1:]

# Iterate over the hex bytes
bitss = [hex_to_bits(hex_byte) for hex_byte in hex_bytes]
# Print the binary strings
print(''.join(bitss))
