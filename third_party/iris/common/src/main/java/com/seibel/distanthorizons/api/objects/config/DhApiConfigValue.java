/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.objects.config;

import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;
import com.seibel.distanthorizons.coreapi.interfaces.config.IConfigEntry;
import com.seibel.distanthorizons.coreapi.interfaces.config.IConverter;
import com.seibel.distanthorizons.coreapi.util.converters.DefaultConverter;
import java.util.function.Consumer;

public class DhApiConfigValue<coreType, apiType>
implements IDhApiConfigValue<apiType> {
    private final IConfigEntry<coreType> configEntry;
    private final IConverter<coreType, apiType> configConverter;

    public DhApiConfigValue(IConfigEntry<coreType> newConfigEntry) {
        this.configEntry = newConfigEntry;
        this.configConverter = new DefaultConverter<apiType>();
    }

    public DhApiConfigValue(IConfigEntry<coreType> newConfigEntry, IConverter<coreType, apiType> newConverter) {
        this.configEntry = newConfigEntry;
        this.configConverter = newConverter;
    }

    @Override
    public apiType getValue() {
        return this.configConverter.convertToApiType(this.configEntry.get());
    }

    @Override
    public apiType getTrueValue() {
        return this.configConverter.convertToApiType(this.configEntry.getTrueValue());
    }

    @Override
    public apiType getApiValue() {
        return this.configConverter.convertToApiType(this.configEntry.getApiValue());
    }

    @Override
    public boolean setValue(apiType newValue) {
        if (this.configEntry.getAllowApiOverride()) {
            this.configEntry.setApiValue(this.configConverter.convertToCoreType(newValue));
            return true;
        }
        return false;
    }

    @Override
    public boolean clearValue() {
        return this.setValue((apiType)null);
    }

    @Override
    public boolean getCanBeOverrodeByApi() {
        return this.configEntry.getAllowApiOverride();
    }

    @Override
    public apiType getDefaultValue() {
        return this.configConverter.convertToApiType(this.configEntry.getDefaultValue());
    }

    @Override
    public apiType getMaxValue() {
        return this.configConverter.convertToApiType(this.configEntry.getMax());
    }

    @Override
    public apiType getMinValue() {
        return this.configConverter.convertToApiType(this.configEntry.getMin());
    }

    @Override
    public void addChangeListener(Consumer<apiType> onValueChangeFunc) {
        this.configEntry.addValueChangeListener(coreValue -> {
            apiType apiValue = this.configConverter.convertToApiType(coreValue);
            onValueChangeFunc.accept(apiValue);
        });
    }
}

