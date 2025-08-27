// BlockBehaviour.cs
using UnityEngine;

/// <summary>
/// Component attached to placed blocks. Tracks bolts attached and loose-state, damage effects, and dismantling.
/// This is a simple but complete behavior for demo purposes.
/// </summary>
[RequireComponent(typeof(Collider))]
public class BlockBehaviour : MonoBehaviour
{
    [HideInInspector] public BlockDefinition definition;
    [HideInInspector] public GridSize gridSize = GridSize.Small;

    public int boltsAttached = 0;      // number of bolts attached currently
    public int boltDeficit = 0;        // if placed unbolted, how many missing
    public bool isBolted => boltsAttached >= GetRequiredBolts();

    public float health = 100f;
    public float maxHealth = 100f;

    // loose-state behavior settings
    public bool inLooseState = false;
    public float looseShakeIntensity = 0.5f;
    public float looseFallThreshold = 1.5f; // e.g. when parent acceleration too high, drop off

    private Rigidbody rb;
    private Collider col;

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        col = GetComponent<Collider>();
        if (rb == null) {
            rb = gameObject.AddComponent<Rigidbody>();
            rb.isKinematic = true; // default to kinematic when attached to ship / grid parent
        }
    }

    void Start()
    {
        // initial health depending on block type / grid size (simple)
        maxHealth = (definition != null && definition.gridSize == GridSize.Large) ? 300f : 100f;
        health = maxHealth;
    }

    public int GetRequiredBolts()
    {
        if (definition == null) return 0;
        return definition.GetRequiredBolts(gridSize);
    }

    public void SetBolted(int count)
    {
        boltsAttached = count;
        boltDeficit = Mathf.Max(0, GetRequiredBolts() - boltsAttached);
        inLooseState = !isBolted;
        UpdateStateAfterBolts();
    }

    void UpdateStateAfterBolts()
    {
        if (isBolted)
        {
            // fully functional
            inLooseState = false;
            // keep kinematic true (attached)
            if (rb) rb.isKinematic = true;
        }
        else
        {
            EnterLooseState();
        }
    }

    public void EnterLooseState()
    {
        inLooseState = true;
        // disable functionality: (you could disable specific components like thrusters)
        // make physics reactive so block may jiggle/fall
        if (rb) rb.isKinematic = false;
        // small shaking impulse to indicate instability
        if (rb) rb.AddTorque(Random.onUnitSphere * looseShakeIntensity, ForceMode.Impulse);
        // optionally start a coroutine for visual shake - simple placeholder:
        StartCoroutine(LooseStatePulse());
    }

    System.Collections.IEnumerator LooseStatePulse()
    {
        float t = 0f;
        while (inLooseState)
        {
            transform.localRotation = Quaternion.Euler(Mathf.Sin(Time.time * 6f) * looseShakeIntensity, Mathf.Sin(Time.time*5f)*looseShakeIntensity, 0f);
            yield return null;
        }
        // reset rotation
        transform.localRotation = Quaternion.identity;
    }

    // Called by welder/repair system to apply bolt additions
    public void AddBoltsFromInventory(Inventory inv)
    {
        if (definition == null) return;
        int required = GetRequiredBolts();
        int missing = Mathf.Max(0, required - boltsAttached);
        if (missing <= 0) return;
        // determine from inv whether we use small or large bolts
        GridSize gs = gridSize;
        int available = (gs == GridSize.Small) ? inv.smallBolts : inv.largeBolts;
        int toTake = Mathf.Min(available, missing);
        bool ok = inv.ConsumeBolts(gs, toTake);
        if (ok)
        {
            boltsAttached += toTake;
            boltDeficit = Mathf.Max(0, required - boltsAttached);
            if (boltDeficit == 0) { inLooseState = false; rb.isKinematic = true; }
        }
    }

    // Dismantle/unbolt returns 90% bolts to inventory
    public void Dismantle(Inventory inv)
    {
        int required = GetRequiredBolts();
        int removeCount = boltsAttached;
        boltsAttached = 0;
        boltDeficit = required;
        inLooseState = true;
        if (inv != null)
        {
            inv.RecoverBolts(gridSize, removeCount, 0.9f);
        }
        // optionally: drop components as resources
        Destroy(gameObject);
    }

    // Example damage function used by drones or weapons
    public void Damage(float dmg)
    {
        health -= dmg;
        if (health <= 0f)
        {
            // when destroyed, release bolts (no recovery) and break apart
            boltsAttached = 0;
            boltDeficit = 0;
            // turn into dynamic debris
            if (rb) { rb.isKinematic = false; rb.AddExplosionForce(200f, transform.position + Random.onUnitSphere * 0.5f, 3f); }
            // play sparks, particles (hook here)
            Destroy(gameObject, 5f);
        }
        else
        {
            // if bolts are missing and health is reduced, more likely to fall off
            if (inLooseState)
            {
                if (rb) rb.AddForce(Random.onUnitSphere * dmg * 0.5f, ForceMode.Impulse);
            }
        }
    }
}