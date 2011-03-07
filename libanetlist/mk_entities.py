#!/usr/bin/python

import os, collections, string, sys

Entity = collections.namedtuple('Entity', 'name attributes inputs outputs')

primitives = [
	Entity(
		name = "IBUF",
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "OBUF",
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "BUFGP",
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "FD",
		attributes = [],
		inputs = [(1, "C"), (1, "D")],
		outputs = [(1, "Q")]
	),
	Entity(
		name = "FDE",
		attributes = [],
		inputs = [(1, "C"), (1, "CE"), (1, "D")],
		outputs = [(1, "Q")]
	),
	Entity(
		name = "VCC",
		attributes = [],
		inputs = [],
		outputs = [(1, "P")]
	),
	Entity(
		name = "GND",
		attributes = [],
		inputs = [],
		outputs = [(1, "G")]
	),
	Entity(
		name = "LUT1",
		attributes = [("INIT", "0")],
		inputs = [(1, "I0")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT2",
		attributes = [("INIT", "0")],
		inputs = [(1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT3",
		attributes = [("INIT", "00")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT4",
		attributes = [("INIT", "0000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT5",
		attributes = [("INIT", "00000000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3"), (1, "I4")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT6",
		attributes = [("INIT", "0000000000000000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3"), (1, "I4"), (1, "I5")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "MUXF7",
		attributes = [],
		inputs = [(1, "S"), (1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "MUXF8",
		attributes = [],
		inputs = [(1, "S"), (1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "MUXCY",
		attributes = [],
		inputs = [(1, "S"), (1, "DI"), (1, "CI")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "XORCY",
		attributes = [],
		inputs = [(1, "LI"), (1, "CI")],
		outputs = [(1, "O")]
	),
]

sites = [
	Entity(
		name = "IOBM",
		attributes = [],
		inputs = [(1, "T"), (1, "O"), (1, "DIFFI_N"), (1, "DIFFO_N")],
		outputs = [(1, "PADOUT"), (1, "PCI_RDY"), (1, "I"), (1, "DIFFO_OUT")]
	),
]

def print_io_list(l):
	for e in l:
		if e[0] > 1:
			for i in range(e[0]):
				print "\t\"%s_%d\"," % (e[1], i)
		else:
			print "\t\"%s\"," % e[1]

def io_count(l):
	c = 0
	for e in l:
		c += e[0]
	return c

def generate_c(array, name):
	print "#include <stdlib.h>"
	print "#include <anetlist/net.h>"
	print "#include <anetlist/%ss.h>" % name
	for p in array:
		print "\n/* %s */" % p.name
		if p.attributes:
			print "static char *%s_attribute_names[] = {" % p.name
			for attr in p.attributes:
				print "\t\"%s\"," % attr[0]
			print "};"

			print "static char *%s_attribute_defaults[] = {" % p.name
			for attr in p.attributes:
				print "\t\"%s\"," % attr[1]
			print "};"

		if p.inputs:
			print "static char *%s_inputs[] = {" % p.name
			print_io_list(p.inputs)
			print "};"

		if p.outputs:
			print "static char *%s_outputs[] = {" % p.name
			print_io_list(p.outputs)
			print "};"

	print "\n\nstruct anetlist_entity anetlist_%ss[] = {" % name
	for p in array:
		print "\t{"
		print "\t\t.type = ANETLIST_ENTITY_INTERNAL,"
		print "\t\t.name = \"%s\"," % p.name
		print "\t\t.attribute_count = %d," % len(p.attributes)
		if p.attributes:
			print "\t\t.attribute_names = %s_attribute_names," % p.name
			print "\t\t.default_attributes = %s_attribute_defaults," % p.name
		else:
			print "\t\t.attribute_names = NULL,"
			print "\t\t.default_attributes = NULL,"
		print "\t\t.inputs = %d," % io_count(p.inputs)
		if p.inputs:
			print "\t\t.input_names = %s_inputs," % p.name
		else:
			print "\t\t.input_names = NULL,"
		print "\t\t.outputs = %d," % io_count(p.outputs)
		if p.outputs:
			print "\t\t.output_names = %s_outputs" % p.name
		else:
			print "\t\t.output_names = NULL"
		print "\t},"
	print "};"

def print_io_enum(prefix, l):
	n = 0
	for e in l:
		if e[0] > 1:
			for i in range(e[0]):
				print "%s%s_%d = %d" % (prefix, e[1], i, n)
				n += 1
		else:
			print "%s%s = %d," % (prefix, e[1], n)
			n += 1

def generate_h(array, name):
	print "#ifndef __ANETLIST_%sS_H" % string.upper(name)
	print "#define __ANETLIST_%sS_H\n" % string.upper(name)

	i = 0
	print "enum {"
	for p in array:
		print "\tANETLIST_%s_%s = %d," % (string.upper(name), p.name, i)
		i += 1
	print "};"

	for p in array:
		if p.inputs:
			print "\n/* %s: inputs */" % p.name
			print "enum {"
			print_io_enum(("\tANETLIST_%s_%s_" % (string.upper(name), p.name)), p.inputs)
			print "};"
		if p.outputs:
			print "/* %s: outputs */" % p.name
			print "enum {"
			print_io_enum(("\tANETLIST_%s_%s_" % (string.upper(name), p.name)), p.outputs)
			print "};"

	print "\nextern struct anetlist_entity anetlist_primitives[];\n"
	
	print "#endif /* __ANETLIST_%sS_H */" % string.upper(name)


if len(sys.argv) != 2:
	print "Usage: %s c|h" % os.path.basename(sys.argv[0])
	sys.exit(1)

if sys.argv[1][0] == "p":
	array = primitives
	name = "primitive"
else:
	array = sites
	name = "site"

print "/* Generated automatically by %s. Do not edit manually! */" % os.path.basename(sys.argv[0])
if sys.argv[1][1] == "c":
	generate_c(array, name)
else:
	generate_h(array, name)
