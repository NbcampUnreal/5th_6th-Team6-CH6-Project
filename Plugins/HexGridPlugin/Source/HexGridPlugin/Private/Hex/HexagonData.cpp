// Fill out your copyright notice in the Description page of Project Settings.


#include "Hex/HexagonData.h"

const FHexCoord FHexCoord::Directions[6] =
{
	FHexCoord( 1,-1, 0),
	FHexCoord( 1, 0,-1),
	FHexCoord( 0, 1,-1),
	FHexCoord(-1, 1, 0),
	FHexCoord(-1, 0, 1),
	FHexCoord( 0,-1, 1)
};
