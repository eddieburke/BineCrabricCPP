/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.override.worldGenerator;

import com.seibel.distanthorizons.api.enums.EDhApiDetailLevel;
import com.seibel.distanthorizons.api.enums.worldGeneration.EDhApiDistantGeneratorMode;
import com.seibel.distanthorizons.api.enums.worldGeneration.EDhApiWorldGeneratorReturnType;
import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.api.objects.data.DhApiChunk;
import java.io.Closeable;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.function.Consumer;

public interface IDhApiWorldGenerator
extends Closeable,
IDhApiOverrideable {
    default public byte getSmallestDataDetailLevel() {
        return EDhApiDetailLevel.BLOCK.detailLevel;
    }

    default public byte getLargestDataDetailLevel() {
        return EDhApiDetailLevel.BLOCK.detailLevel;
    }

    default public byte getMinGenerationGranularity() {
        return EDhApiDetailLevel.CHUNK.detailLevel;
    }

    default public byte getMaxGenerationGranularity() {
        return (byte)(EDhApiDetailLevel.CHUNK.detailLevel + 2);
    }

    public boolean isBusy();

    default public CompletableFuture<Void> generateChunks(int chunkPosMinX, int chunkPosMinZ, byte granularity, byte targetDataDetail, EDhApiDistantGeneratorMode generatorMode, ExecutorService worldGeneratorThreadPool, Consumer<Object[]> resultConsumer) {
        throw new UnsupportedOperationException();
    }

    default public CompletableFuture<Void> generateApiChunks(int chunkPosMinX, int chunkPosMinZ, byte granularity, byte targetDataDetail, EDhApiDistantGeneratorMode generatorMode, ExecutorService worldGeneratorThreadPool, Consumer<DhApiChunk> resultConsumer) {
        throw new UnsupportedOperationException();
    }

    default public EDhApiWorldGeneratorReturnType getReturnType() {
        return EDhApiWorldGeneratorReturnType.VANILLA_CHUNKS;
    }

    public void preGeneratorTaskStart();

    @Override
    public void close();
}

