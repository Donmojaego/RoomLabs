static smk::Params toParams (const float* v)
{
    smk::Params q;
    q.model = (smk::Model) (int) v[0]; q.tone = v[1]; q.space = v[2]; q.mod = v[3]; q.dream = v[4]; q.clean = v[5]; q.levelDb = v[6];
    q.attackMs = v[7]; q.decayMs = v[8]; q.sustain = v[9]; q.releaseMs = v[10]; q.bright = v[11]; q.tremolo = v[12]; q.riseS = v[13];
    q.glideMs = v[14]; q.chord = (int) v[15]; q.tune = v[16];
    return q;
}
