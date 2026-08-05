#define MAT_ID_OPAQUE 1
#define MAT_ID_GRASS 2
#define MAT_ID_LEAVES 3
#define MAT_ID_HAND 4
#define MAT_ID_ENTITY_PLAYER 5
#define MAT_ID_WATER 6
#define MAT_ID_STAINED_GLASS 7
#define MAT_ID_ICE 8
#define MAT_ID_TORCH 30
#define MAT_ID_LAVA 31
#define MAT_ID_GLOWSTONE 32
#define MAT_ID_FIRE 33
#define MAT_ID_LIT_FURNACE 34



struct MaterialMask
{
	float sky;
	float opaque;
	float grass;
	float leaves;
	float hand;
	float entityPlayer;
	float water;
	float stainedGlass;
	float ice;
	float torch;
	float lava;
	float glowstone;
	float fire;
	float litFurnace;
};


float GetMaterialMask(const in int ID, in float matID) 
{
	//Catch last part of sky
	if (matID > 254.0f) 
	{
		matID = 0.0f;
	}

	if (matID == ID) 
	{
		return 1.0f;
	} 
	else 
	{
		return 0.0f;
	}
}

MaterialMask CalculateMasks(float materialID)
{
	MaterialMask masks;

	materialID *= 255.0;

	// if (isEyeInWater > 0)
	// {
	// 	// masks.sky = 0.0f;
	// }
	// else
	{
		masks.sky = 0.0;
		if (texture2D(depthtex1, texcoord.st).x > 0.999999)
		{
			masks.sky = 1.0;
		}
	}
		//masks.sky = GetMaterialMask(0, materialID);
		//masks.sky = texture2D(depthtex1, texcoord).x > 0.999999 ? 1.0 : 0.0;



	masks.opaque 		= GetMaterialMask(MAT_ID_OPAQUE, materialID);
	masks.grass 			= GetMaterialMask(MAT_ID_GRASS, materialID);
	masks.leaves 		= GetMaterialMask(MAT_ID_LEAVES, materialID);
	masks.hand 			= GetMaterialMask(MAT_ID_HAND, materialID);
	masks.entityPlayer 	= GetMaterialMask(MAT_ID_ENTITY_PLAYER, materialID);
	masks.water 			= GetMaterialMask(MAT_ID_WATER, materialID);
	masks.stainedGlass	= GetMaterialMask(MAT_ID_STAINED_GLASS, materialID);
	masks.ice 			= GetMaterialMask(MAT_ID_ICE, materialID);
	masks.torch 			= GetMaterialMask(MAT_ID_TORCH, materialID);
	masks.lava 			= GetMaterialMask(MAT_ID_LAVA, materialID);
	masks.glowstone 		= GetMaterialMask(MAT_ID_GLOWSTONE, materialID);
	masks.fire 			= GetMaterialMask(MAT_ID_FIRE, materialID);
	masks.litFurnace 	= GetMaterialMask(MAT_ID_LIT_FURNACE, materialID);

	return masks;
}
