// -------------------- template instantiations -------------------

template class Uint8ConfigParameter<Config>;
template class Uint16ConfigParameter<Config>;
template class BooleanConfigParameter<Config>;
template class CharArrayConfigParameter<Config>;
template class EnumConfigParameter<Config, uint8_t, BuzzerMode>;
template class EnumConfigParameter<Config, uint8_t, SleepModeOledLed>;