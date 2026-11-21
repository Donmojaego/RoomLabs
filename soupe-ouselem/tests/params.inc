// conversion d'une ligne de factory_presets.inc en soup::Params (partagé par les outils de test)
static soup::Params toParams (const float* v)
{
    soup::Params q;
    q.inst = (soup::Inst) (int) v[0]; q.fx = (soup::Fx) (int) v[1]; q.division = (int) v[2]; q.reverseOn = v[3] > 0.5f; q.stutterOn = v[4] > 0.5f;
    q.mix = v[5]; q.feedback = v[6]; q.stretch = v[7]; q.tone = v[8]; q.damp = v[9]; q.space = v[10]; q.levelDb = v[11];
    q.attackMs = v[12]; q.decayMs = v[13]; q.sustain = v[14]; q.releaseMs = v[15]; q.brightness = v[16]; q.tune = v[17];
    q.arpOn = v[18] > 0.5f; q.arpMode = (int) v[19]; q.arpOct = (int) v[20];
    return q;
}
