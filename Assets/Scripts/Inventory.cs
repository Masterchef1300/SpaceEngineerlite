// Inventory.cs
using UnityEngine;
using System.Collections.Generic;

/// <summary>
/// Simple inventory for refined resources and crafted bolts.
/// This is intentionally straightforward — expand later to stacks, capacities, cargo containers.
/// </summary>
public class Inventory : MonoBehaviour
{
    // refined resources: ingots / plates etc. Use string keys for flexibility.
    private Dictionary<string, int> resources = new Dictionary<string, int>();

    // bolts separated into small/large bolts (counts)
    public int smallBolts = 0;
    public int largeBolts = 0;

    public void AddResource(string key, int amount)
    {
        if (amount <= 0) return;
        if (!resources.ContainsKey(key)) resources[key] = 0;
        resources[key] += amount;
    }

    public bool HasResource(string key, int amount)
    {
        if (!resources.ContainsKey(key)) return false;
        return resources[key] >= amount;
    }

    public bool ConsumeResource(string key, int amount)
    {
        if (!HasResource(key, amount)) return false;
        resources[key] -= amount;
        return true;
    }

    public int GetResourceAmount(string key)
    {
        if (!resources.ContainsKey(key)) return 0;
        return resources[key];
    }

    public Dictionary<string,int> GetAllResources()
    {
        // shallow copy for UI
        return new Dictionary<string,int>(resources);
    }

    // Bolt helper functions
    public void AddBolts(GridSize gridSize, int amount)
    {
        if (gridSize == GridSize.Small) smallBolts += amount;
        else largeBolts += amount;
    }

    public bool HasBolts(GridSize gridSize, int required)
    {
        return (gridSize == GridSize.Small) ? (smallBolts >= required) : (largeBolts >= required);
    }

    public bool ConsumeBolts(GridSize gridSize, int required)
    {
        if (!HasBolts(gridSize, required)) return false;
        if (gridSize == GridSize.Small) smallBolts -= required;
        else largeBolts -= required;
        return true;
    }

    // Recovery on unbolt: add percentage of required bolts back
    public void RecoverBolts(GridSize gridSize, int count, float percent = 0.9f)
    {
        int recovered = Mathf.FloorToInt(count * percent);
        AddBolts(gridSize, recovered);
    }
}