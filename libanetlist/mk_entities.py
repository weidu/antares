#!/usr/bin/python

import os, collections, string, sys

Entity = collections.namedtuple('Entity', 'name where attributes inputs outputs')

# the "where" array defines if the entity can be a primitive, a BEL or a site (in this order)
entities = [
# GND/VCC
	Entity(
		name = "VCC",
		where = [True, True, True],
		attributes = [],
		inputs = [],
		outputs = [(1, "P")]
	),
	Entity(
		name = "GND",
		where = [True, True, True],
		attributes = [],
		inputs = [],
		outputs = [(1, "G")]
	),

# I/O and clock buffers
	Entity(
		name = "IBUF",
		where = [True, False, False],
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "OBUF",
		where = [True, False, False],
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "IOBM",
		where = [True, True, True],
		attributes = [
			("IN_TERM", "NONE"), ("OUT_TERM", "NONE"), ("DIFF_TERM", "FALSE"),
			("PULLTYPE", "NONE"),
			("DRIVE_STRENGTH", "12"), ("SLEW_RATE", "SLOW"),
			("SUSPEND", "DRIVE_LAST_VALUE"),
			("IPROGRAMMING", ""), ("ISTANDARD", "LVCMOS33"),
			("OPROGRAMMING", ""), ("OSTANDARD", "LVCMOS33")
		],
		inputs = [(1, "T"), (1, "O"), (1, "DIFFI_IN")],
		outputs = [(1, "PADOUT"), (1, "PCI_RDY"), (1, "I"), (1, "DIFFO_OUT")]
	),
	Entity(
		name = "IOBS",
		where = [True, True, True],
		attributes = [
			("IN_TERM", "NONE"), ("OUT_TERM", "NONE"), ("DIFF_TERM", "FALSE"),
			("PULLTYPE", "NONE"),
			("DRIVE_STRENGTH", "12"), ("SLEW_RATE", "SLOW"),
			("SUSPEND", "DRIVE_LAST_VALUE"),
			("IPROGRAMMING", ""), ("ISTANDARD", "LVCMOS33"),
			("OPROGRAMMING", ""), ("OSTANDARD", "LVCMOS33")
		],
		inputs = [(1, "T"), (1, "O"), (1, "DIFFI_IN"), (1, "DIFFO_IN")],
		outputs = [(1, "PADOUT"), (1, "PCI_RDY"), (1, "I"), (1, "DIFFO_OUT")]
	),
	Entity(
		name = "BUFGP",
		where = [True, False, False],
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "BUFGMUX",
		where = [True, True, True],
		attributes = [("CLK_SEL_TYPE", "SYNC")],
		inputs = [(1, "S"), (1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),

# Flip-flops
	Entity(
		name = "FD",
		where = [True, False, False],
		attributes = [],
		inputs = [(1, "C"), (1, "D")],
		outputs = [(1, "Q")]
	),
	Entity(
		name = "FDE",
		where = [True, False, False],
		attributes = [],
		inputs = [(1, "C"), (1, "CE"), (1, "D")],
		outputs = [(1, "Q")]
	),
	# FDRE is the largest subset of flip-flop features we support atm, so we use that as BEL.
	Entity(
		name = "FDRE",
		where = [True, True, False],
		attributes = [],
		inputs = [(1, "C"), (1, "CE"), (1, "D"), (1, "R")],
		outputs = [(1, "Q")]
	),

# LUTs
	Entity(
		name = "LUT1",
		where = [True, False, False],
		attributes = [("INIT", "0")],
		inputs = [(1, "I0")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT2",
		where = [True, False, False],
		attributes = [("INIT", "0")],
		inputs = [(1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT3",
		where = [True, False, False],
		attributes = [("INIT", "00")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT4",
		where = [True, False, False],
		attributes = [("INIT", "0000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT5",
		where = [True, False, False],
		attributes = [("INIT", "00000000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3"), (1, "I4")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT6",
		where = [True, False, False],
		attributes = [("INIT", "0000000000000000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3"), (1, "I4"), (1, "I5")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "LUT6_2",
		where = [True, True, False],
		attributes = [("INIT", "0000000000000000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3"), (1, "I4"), (1, "I5")],
		outputs = [(1, "O5"), (1, "O6")]
	),
	Entity(
		name = "MUXF7",
		where = [True, True, False],
		attributes = [],
		inputs = [(1, "S"), (1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "MUXF8",
		where = [True, True, False],
		attributes = [],
		inputs = [(1, "S"), (1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),

# Carry chains
	Entity(
		name = "MUXCY",
		where = [True, False, False],
		attributes = [],
		inputs = [(1, "S"), (1, "DI"), (1, "CI")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "XORCY",
		where = [True, False, False],
		attributes = [],
		inputs = [(1, "LI"), (1, "CI")],
		outputs = [(1, "O")]
	),
	Entity(
		name = "CARRY4",
		where = [True, True, False],
		attributes = [], # TODO: CYINIT select
		inputs = [
			(1, "DI0"), (1, "S0"),
			(1, "DI1"), (1, "S1"),
			(1, "DI2"), (1, "S2"),
			(1, "DI3"), (1, "S3"),
			(1, "CYINIT"), (1, "CIN")
		],
		outputs = [
			(1, "O0"), (1, "CO0"),
			(1, "O1"), (1, "CO1"),
			(1, "O2"), (1, "CO2"),
			(1, "O3"), (1, "CO3"),
		]
	),

# Slices
	Entity(
		name = "SLICEX",
		where = [False, False, True],
		attributes = [
			("CLKINV", "CLK"), ("RESET_TYPE", "SYNC"),
			("CEUSED", "0"), ("SRUSED", "0"),
			("AUSED", "0"), ("BUSED", "0"), ("CUSED", "0"), ("DUSED", "0"), 
			("AOUTMUX", "O6"), ("AFFMUX", "O6"),
			("BOUTMUX", "O6"), ("BFFMUX", "O6"),
			("COUTMUX", "O6"), ("CFFMUX", "O6"),
			("DOUTMUX", "O6"), ("DFFMUX", "O6"),
			("AFFSRINIT", ""), ("BFFSRINIT", ""), ("CFFSRINIT", ""), ("DFFSRINIT", ""),
			("A5FFSRINIT", ""), ("B5FFSRINIT", ""), ("C5FFSRINIT", ""), ("D5FFSRINIT", "")
		],
		inputs = [
			(1, "A1"), (1, "A2"), (1, "A3"), (1, "A4"), (1, "A5"), (1, "A6"), (1, "AX"),
			(1, "B1"), (1, "B2"), (1, "B3"), (1, "B4"), (1, "B5"), (1, "B6"), (1, "BX"),
			(1, "C1"), (1, "C2"), (1, "C3"), (1, "C4"), (1, "C5"), (1, "C6"), (1, "CX"),
			(1, "D1"), (1, "D2"), (1, "D3"), (1, "D4"), (1, "D5"), (1, "D6"), (1, "DX"),
			(1, "CLK"), (1, "CE"), (1, "SR")
		],
		outputs = [
			(1, "A"), (1, "AMUX"), (1, "AQ"),
			(1, "B"), (1, "BMUX"), (1, "BQ"),
			(1, "C"), (1, "CMUX"), (1, "CQ"),
			(1, "D"), (1, "DMUX"), (1, "DQ")
		]
	),
	Entity(
		name = "SLICEM",
		where = [False, False, True],
		attributes = [ # TODO: CYINIT select
			("CLKINV", "CLK"), ("RESET_TYPE", "SYNC"),
			("CEUSED", "0"), ("SRUSED", "0"),
			("AUSED", "0"), ("BUSED", "0"), ("CUSED", "0"), ("DUSED", "0"),
			("WA7USED", "0"), ("WA8USED", "0"), ("WEMUX", "WE"),
			("AOUTMUX", "O6"), ("AFFMUX", "O6"),
			("BOUTMUX", "O6"), ("BFFMUX", "O6"),
			("COUTMUX", "O6"), ("CFFMUX", "O6"),
			("DOUTMUX", "O6"), ("DFFMUX", "O6"),
			("COUTUSED", "0"),
			("ADI1MUX", "BMC31"), ("BDI1MUX", "BMC31"), ("CDI1MUX", "BMC31"),
			("AFFSRINIT", ""), ("BFFSRINIT", ""), ("CFFSRINIT", ""), ("DFFSRINIT", ""),
			("A5FFSRINIT", ""), ("B5FFSRINIT", ""), ("C5FFSRINIT", ""), ("D5FFSRINIT", ""),
			("PRECYINIT", "0"), ("ACY0", "O5"), ("BCY0", "O5"), ("CCY0", "O5"), ("DCY0", "O5"),
			("A5LUT", "LUT"), ("A5RAMMODE", ""), ("A6RAMMODE", ""),
			("B5LUT", "LUT"), ("B5RAMMODE", ""), ("B6RAMMODE", ""),
			("C5LUT", "LUT"), ("C5RAMMODE", ""), ("C6RAMMODE", ""),
			("D5LUT", "LUT"), ("D5RAMMODE", ""), ("D6RAMMODE", "")
		],
		inputs = [
			(1, "A1"), (1, "A2"), (1, "A3"), (1, "A4"), (1, "A5"), (1, "A6"), (1, "AX"), (1, "AI"),
			(1, "B1"), (1, "B2"), (1, "B3"), (1, "B4"), (1, "B5"), (1, "B6"), (1, "BX"), (1, "BI"),
			(1, "C1"), (1, "C2"), (1, "C3"), (1, "C4"), (1, "C5"), (1, "C6"), (1, "CX"), (1, "CI"),
			(1, "D1"), (1, "D2"), (1, "D3"), (1, "D4"), (1, "D5"), (1, "D6"), (1, "DX"), (1, "DI"),
			(1, "CLK"), (1, "CE"), (1, "SR"), (1, "WE"),
			(1, "CIN")
		],
		outputs = [
			(1, "A"), (1, "AMUX"), (1, "AQ"),
			(1, "B"), (1, "BMUX"), (1, "BQ"),
			(1, "C"), (1, "CMUX"), (1, "CQ"),
			(1, "D"), (1, "DMUX"), (1, "DQ"),
			(1, "COUT")
		]
	)
]

entity_names = ["primitive", "bel", "site"]

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

def generate_c(ti):
	print "#include <stdlib.h>"
	print "#include <anetlist/entities.h>"
	print "#include <anetlist/%ss.h>" % entity_names[ti]
	for p in entities:
		if p.where[ti]:
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

	print "\n\nstruct anetlist_entity anetlist_%ss[] = {" % entity_names[ti]
	for p in entities:
		if p.where[ti]:
			print "\t{"
			print "\t\t.type = ANETLIST_ENTITY_INTERNAL,"
			print "\t\t.bel = %d," % int(p.where[1])
			print "\t\t.name = \"%s\"," % p.name
			print "\t\t.n_attributes = %d," % len(p.attributes)
			if p.attributes:
				print "\t\t.attribute_names = %s_attribute_names," % p.name
				print "\t\t.default_attributes = %s_attribute_defaults," % p.name
			else:
				print "\t\t.attribute_names = NULL,"
				print "\t\t.default_attributes = NULL,"
			print "\t\t.n_inputs = %d," % io_count(p.inputs)
			if p.inputs:
				print "\t\t.input_names = %s_inputs," % p.name
			else:
				print "\t\t.input_names = NULL,"
			print "\t\t.n_outputs = %d," % io_count(p.outputs)
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

def generate_h(ti):
	print "#ifndef __ANETLIST_%sS_H" % string.upper(entity_names[ti])
	print "#define __ANETLIST_%sS_H\n" % string.upper(entity_names[ti])

	i = 0
	print "enum {"
	for p in entities:
		if p.where[ti]:
			print "\tANETLIST_%s_%s = %d," % (string.upper(entity_names[ti]), p.name, i)
			i += 1
	print "};"

	for p in entities:
		if p.where[ti]:
			if p.inputs:
				print "\n/* %s: inputs */" % p.name
				print "enum {"
				print_io_enum(("\tANETLIST_%s_%s_" % (string.upper(entity_names[ti]), p.name)), p.inputs)
				print "};"
			if p.outputs:
				print "/* %s: outputs */" % p.name
				print "enum {"
				print_io_enum(("\tANETLIST_%s_%s_" % (string.upper(entity_names[ti]), p.name)), p.outputs)
				print "};"

	print "\nextern struct anetlist_entity anetlist_%ss[%d];\n" % (entity_names[ti], i)
	
	print "#endif /* __ANETLIST_%sS_H */" % string.upper(entity_names[ti])


if len(sys.argv) != 3:
	print "Usage: %s <n> c|h" % os.path.basename(sys.argv[0])
	sys.exit(1)

ti = int(sys.argv[1])

print "/* Generated automatically by %s. Do not edit manually! */" % os.path.basename(sys.argv[0])
if sys.argv[2] == "c":
	generate_c(ti)
else:
	generate_h(ti)
