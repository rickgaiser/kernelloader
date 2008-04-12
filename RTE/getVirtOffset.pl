#!/usr/bin/perl
# Copyright (c) 2007 Mega Man
use strict;
my $filename = shift;

open(FD, "-|", "ee-objdump -p $filename") or die "Can't run ee-objdump";
while(<FD>)
{
	if (/LOAD off *0x([0-9a-fA-F]*) vaddr *0x([0-9a-fA-F]*)/)
	{
		printf hex($2) - hex($1);
	}
}
close(FD);
