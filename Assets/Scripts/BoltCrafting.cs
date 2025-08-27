// BoltCrafting.cs
using UnityEngine;
using System.Collections;

/// <summary>
/// Crafts bolts from iron ingots.
/// JSON spec earlier: smallBoltYield=60 per iron, largeBoltYield=50 per iron, ironPerBatch = 1, craftTimeSeconds = 1
/// This component handles crafting coroutines and exposes StartCraftBolts() to UI.
/// Attach to a Refinery, Assembler, or Player object (depending on gameplay).
/// </summary>
public class BoltCrafting : MonoBehaviour
{
    [Header("Bolt Crafting Settings (per JSON spec)")]
    public int smallBoltYield = 60;
    public int largeBoltYield = 50;
    public int ironPerBatch = 1;
    public float craftTimeSeconds = 1.0f;

    public Inventory linkedInventory;

    private bool crafting = false;

    private void Reset()
    {
        // Try auto-link inventory if on same GameObject
        linkedInventory = GetComponent<Inventory>();
    }

    public bool CanCraftSmall()
    {
        return linkedInventory != null && linkedInventory.HasResource("IronIngot", ironPerBatch);
    }

    public bool CanCraftLarge()
    {
        return linkedInventory != null && linkedInventory.HasResource("IronIngot", ironPerBatch);
    }

    public void StartCraftSmallBolts(int batches = 1)
    {
        if (!CanCraftSmall()) return;
        StartCoroutine(CraftSmallCoroutine(batches));
    }

    public void StartCraftLargeBolts(int batches = 1)
    {
        if (!CanCraftLarge()) return;
        StartCoroutine(CraftLargeCoroutine(batches));
    }

    private IEnumerator CraftSmallCoroutine(int batches)
    {
        if (crafting) yield break;
        crafting = true;
        for (int i = 0; i < batches; i++)
        {
            // check each batch
            if (!linkedInventory.HasResource("IronIngot", ironPerBatch)) break;
            linkedInventory.ConsumeResource("IronIngot", ironPerBatch);
            yield return new WaitForSeconds(craftTimeSeconds);
            linkedInventory.AddBolts(GridSize.Small, smallBoltYield);
        }
        crafting = false;
    }

    private IEnumerator CraftLargeCoroutine(int batches)
    {
        if (crafting) yield break;
        crafting = true;
        for (int i = 0; i < batches; i++)
        {
            if (!linkedInventory.HasResource("IronIngot", ironPerBatch)) break;
            linkedInventory.ConsumeResource("IronIngot", ironPerBatch);
            yield return new WaitForSeconds(craftTimeSeconds);
            linkedInventory.AddBolts(GridSize.Large, largeBoltYield);
        }
        crafting = false;
    }
}