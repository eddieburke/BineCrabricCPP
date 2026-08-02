/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.coreapi.DependencyInjection;

import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IBindable;
import java.util.ArrayList;

public class OverridePriorityListContainer
implements IBindable {
    private final ArrayList<OverridePriorityPair> overridePairList = new ArrayList();

    public void addOverride(IDhApiOverrideable override) {
        OverridePriorityPair priorityPair = new OverridePriorityPair(override, override.getPriority());
        this.overridePairList.add(priorityPair);
        this.sortList();
    }

    public boolean removeOverride(IDhApiOverrideable override) {
        boolean overrideRemoved = this.overridePairList.removeIf(pair -> pair.override.equals(override));
        return overrideRemoved;
    }

    public IDhApiOverrideable getOverrideWithLowestPriority() {
        if (this.overridePairList.size() == 0) {
            return null;
        }
        return this.overridePairList.get((int)(this.overridePairList.size() - 1)).override;
    }

    public IDhApiOverrideable getOverrideWithHighestPriority() {
        if (this.overridePairList.size() != 0) {
            return this.overridePairList.get((int)0).override;
        }
        return null;
    }

    public IDhApiOverrideable getCoreOverride() {
        int lastIndex = this.overridePairList.size() - 1;
        if (this.overridePairList.get(lastIndex) != null && this.overridePairList.get((int)lastIndex).priority == -1) {
            return this.overridePairList.get((int)lastIndex).override;
        }
        return null;
    }

    public IDhApiOverrideable getOverrideWithPriority(int priority) {
        for (OverridePriorityPair pair : this.overridePairList) {
            if (pair.priority != priority) continue;
            return pair.override;
        }
        return null;
    }

    private void sortList() {
        this.overridePairList.sort((x, y) -> Integer.compare(y.priority, x.priority));
    }

    private class OverridePriorityPair {
        public final IDhApiOverrideable override;
        public int priority;

        public OverridePriorityPair(IDhApiOverrideable newOverride, int newPriority) {
            this.override = newOverride;
            this.priority = newPriority;
        }
    }
}

