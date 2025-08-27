// BlockDefinition.cs
using UnityEngine;
using System.Collections.Generic;

[CreateAssetMenu(menuName = "SpaceLite/BlockDefinition", fileName = "NewBlockDef")]
public class BlockDefinition : ScriptableObject
{
    public string blockName = "NewBlock";
    public BlockCategory category = BlockCategory.Structure;
    public GridSize gridSize = GridSize.Small;

    [Header("Visual / Prefab")]
    public GameObject prefab; // prefab to instantiate when placed (must include BlockBehaviour)

    [Header("Crafting - refined components")]
    // simple recipe entries: "component name" -> required amount (all quantities are refined materials or components)
    public List<string> recipeNames = new List<string>();
    public List<int> recipeAmounts = new List<int>();

    [Header("Bolt Requirements")]
    public int baseSmallGridBolts = 4;    // bolts for small grid by default
    public int baseLargeGridBolts = 6;    // bolts for large grid by default

    // Optional per-block override for small & large:
    public int overrideSmallGridBolts = -1;
    public int overrideLargeGridBolts = -1;

    public int GetRequiredBolts(GridSize requestedGrid)
    {
        if (requestedGrid == GridSize.Small)
            return overrideSmallGridBolts >= 0 ? overrideSmallGridBolts : baseSmallGridBolts;
        else
            return overrideLargeGridBolts >= 0 ? overrideLargeGridBolts : baseLargeGridBolts;
    }

    public bool HasRecipe()
    {
        return recipeNames.Count == recipeAmounts.Count && recipeNames.Count > 0;
    }
}