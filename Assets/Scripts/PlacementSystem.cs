// PlacementSystem.cs
using UnityEngine;

/// <summary>
/// Example placement system:
/// - Checks material recipe availability
/// - Checks bolts
/// - If bolts insufficient, places block in LOSE/UNBOLTED state (BlockBehaviour handles consequences)
/// - If bolts sufficient, consumes bolts and marks block as bolted
/// 
/// Hook this to your placement UI: call TryPlaceBlock(definition, worldPosition)
/// </summary>
public class PlacementSystem : MonoBehaviour
{
    public Inventory playerInventory; // assign in inspector or find at runtime

    // place block and return instantiated GameObject (or null)
    public GameObject TryPlaceBlock(BlockDefinition def, Vector3 worldPos, GridSize placingGrid)
    {
        if (def == null || def.prefab == null)
        {
            Debug.LogWarning("BlockDefinition or prefab missing.");
            return null;
        }

        // 1) Check recipe (consume refined components)
        if (def.HasRecipe())
        {
            // require that all recipe components exist in inventory
            for (int i = 0; i < def.recipeNames.Count; i++)
            {
                string name = def.recipeNames[i];
                int amount = def.recipeAmounts[i];
                if (!playerInventory.HasResource(name, amount))
                {
                    Debug.Log("Missing resource: " + name + " x" + amount);
                    // optionally: show message to user
                    return null;
                }
            }
            // consume recipe
            for (int i = 0; i < def.recipeNames.Count; i++)
            {
                playerInventory.ConsumeResource(def.recipeNames[i], def.recipeAmounts[i]);
            }
        }

        // 2) Bolt requirements
        int requiredBolts = def.GetRequiredBolts(placingGrid);
        bool hasBolts = playerInventory.HasBolts(placingGrid, requiredBolts);

        // instantiate block
        GameObject go = Instantiate(def.prefab, worldPos, Quaternion.identity);
        BlockBehaviour bb = go.GetComponent<BlockBehaviour>();
        if (bb == null)
        {
            bb = go.AddComponent<BlockBehaviour>();
        }
        bb.definition = def;
        bb.gridSize = placingGrid;

        if (hasBolts)
        {
            // consume bolts and mark bolted
            playerInventory.ConsumeBolts(placingGrid, requiredBolts);
            bb.SetBolted(requiredBolts);
        }
        else
        {
            // leave unbolted with bolt deficit
            bb.SetBolted(0);
            bb.boltDeficit = requiredBolts;
            bb.EnterLooseState();
            // show UI: requires bolts
            Debug.Log($"Placed unbolted: needs {requiredBolts} {placingGrid} bolts. You have smallBolts={playerInventory.smallBolts}, largeBolts={playerInventory.largeBolts}");
        }

        return go;
    }
}